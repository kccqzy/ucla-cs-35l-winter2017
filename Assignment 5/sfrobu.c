/* -*- mode: c; c-basic-offset: 2; -*- */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static inline void *
xrealloc(void *p, size_t size)
{
  void *r = realloc(p, size);
  if (!r) {
    perror("realloc()");
    abort();
  }
  return r;
}

static int
frobcmp(uint8_t const *const *aa, uint8_t const *const *ba)
{
  uint8_t const *a = *aa, *b = *ba;
  while (*a != ' ' && *b != ' ') {
    uint8_t ca = *a ^ 0x2a;
    uint8_t cb = *b ^ 0x2a;
    if (ca == cb) {
      ++a;
      ++b;
    } else
      return (int) ca - (int) cb;
  }
  if (*a == ' ' && *b == ' ')
    return 0;
  else if (*a == ' ')
    return -1;
  else
    return 1;
}

static int
frobcmp_ci(uint8_t const *const *aa, uint8_t const *const *ba)
{
  uint8_t const *a = *aa, *b = *ba;
  while (*a != ' ' && *b != ' ') {
    uint8_t ca = toupper(*a ^ 0x2a);
    uint8_t cb = toupper(*b ^ 0x2a);
    if (ca == cb) {
      ++a;
      ++b;
    } else
      return (int) ca - (int) cb;
  }
  if (*a == ' ' && *b == ' ')
    return 0;
  else if (*a == ' ')
    return -1;
  else
    return 1;
}

static inline size_t
round_up(size_t i)
{
  return i & 0xffffull ? ((i >> 16) + 1) << 16 : i;
}

static inline size_t
max(size_t a, size_t b)
{
  return a > b ? a : b;
}

static size_t
read_once(uint8_t **buf, size_t *capacity, size_t *bytes_read,
          size_t bytes_requested, const char *argv0)
{
  if (*bytes_read + bytes_requested + 1 > *capacity) {
    size_t new_capacity = round_up(
      max(*bytes_read + bytes_requested + 1, *capacity + (*capacity >> 2)));
    // Expand the buffer by an amount proportional to the current size, giving
    // us amortized linear-time behavior. Use a less-than-double growth factor
    // to avoid excessive allocation. When size is small, use linear growth.
    *buf = (uint8_t *) xrealloc(*buf, new_capacity);
    *capacity = new_capacity;
  }
  while (1) {
    ssize_t r = read(STDIN_FILENO, *buf + *bytes_read, bytes_requested);
    if (r == 0) {
      return 0;
    } else if (r == -1) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      else {
        fprintf(stderr, "%s: could not read stdin: %s\n", argv0,
                strerror(errno));
        exit(1);
      }
    } else {
      *bytes_read += r;
      return r;
    }
  }
}

static inline size_t
read_all_stdin(size_t stat_size, uint8_t **buf, const char *argv0)
{
  size_t capacity = 0;
  size_t bytes_read = 0;
  *buf = (uint8_t *) xrealloc(NULL, round_up(stat_size + 1));
  if (stat_size & ~0xffffull)
    read_once(buf, &capacity, &bytes_read, stat_size & ~0xffffull, argv0);
  // This apparently imitates Python's sys.stdin.read().
  size_t r = 0;
  do {
    r = read_once(buf, &capacity, &bytes_read, 0x10000ull, argv0);
  } while (r);
  assert(capacity > bytes_read);
  return bytes_read;
}

static inline const uint8_t *
find_space(const uint8_t *begin, const uint8_t *end)
{
  // It is a bad idea to return NULL when it is not found. Use C++'s convention
  // of searching in [begin, end).
  uint8_t *p = (uint8_t *) memchr(begin, ' ', end - begin);
  return p ? p : end;
}

static inline size_t
split(const uint8_t *buf, size_t size, const uint8_t ***locations)
{
  assert(!*locations); // locations.empty()
  if (size) {
    size_t locations_capacity = 0xffffull / sizeof(uint8_t *);
    *locations = (const uint8_t **) xrealloc(NULL, 0xffffull);
    **locations = buf;
    size_t locations_size = 1; // locations.assign(1, buf)
    const uint8_t *p = buf + 1;
    while (1) {
      p = find_space(p, buf + size) + 1;
      if (p < buf + size) {
        if (locations_size == locations_capacity) {
          size_t new_capacity =
            round_up((locations_capacity + (locations_capacity >> 2)) *
                     sizeof(uint8_t *)) /
            sizeof(uint8_t *);
          *locations = (const uint8_t **) xrealloc(
            *locations, new_capacity * sizeof(uint8_t *));
          locations_capacity = new_capacity;
        }
        (*locations)[locations_size++] = p; // locations.emplace_back(p)
      } else
        return locations_size;
    }
  }
  return 0;
}

static inline void
prepare_outbuf(const uint8_t *buf, uint8_t **outbuf, size_t size,
               const uint8_t *const *locations, size_t locations_size)
{
  *outbuf = (uint8_t *) xrealloc(NULL, size);
  uint8_t *outp = *outbuf;
  for (size_t i = 0; i < locations_size; ++i) {
    const uint8_t *p = locations[i];
    size_t this_size = find_space(p, buf + size) - p + 1;
    memcpy(outp, p, this_size);
    outp += this_size;
  }
}

static inline void
write_all_stdout(const uint8_t *outbuf, size_t size, const char *argv0)
{
  size_t written = 0;
  do {
    ssize_t w = write(STDOUT_FILENO, outbuf, size - written);
    if (w == -1) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      else {
        fprintf(stderr, "%s: could not write stdout: %s\n", argv0,
                strerror(errno));
        exit(1);
      }
    } else
      written += w;
  } while (written < size);
}

int
main(int argc, const char **argv)
{
  assert(argc >= 1);
  bool ignore_case;
  if (argc == 1) {
    ignore_case = false;
  } else if (argc == 2 && 0 == strcmp("-f", argv[1])) {
    ignore_case = true;
  } else {
    fprintf(stderr,
            "%s: invalid arguments\n"
            "usage: %s [-f]\n",
            argv[0], argv[0]);
    exit(1);
  }

  struct stat stdin_stat;
  size_t initial_size =
    fstat(STDIN_FILENO, &stdin_stat) == 0 && stdin_stat.st_size >= 0
      ? stdin_stat.st_size
      : 0;
  uint8_t *buf = NULL;
  size_t size = read_all_stdin(initial_size, &buf, argv[0]);
  if (size && buf[size - 1] != ' ') buf[size++] = ' ';

  const uint8_t **locations = NULL;
  size_t locations_size = split(buf, size, &locations);
  qsort(
    locations, locations_size, sizeof(const uint8_t *),
    (int (*)(const void *, const void *))(ignore_case ? frobcmp_ci : frobcmp));
  uint8_t *outbuf = NULL;
  prepare_outbuf(buf, &outbuf, size, locations, locations_size);
  free(buf);
  free(locations);

  write_all_stdout(outbuf, size, argv[0]);
  free(outbuf);

  return 0;
}
