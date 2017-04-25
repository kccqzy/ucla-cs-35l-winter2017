/* -*- mode: c; c-basic-offset: 2; -*- */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static inline int
frobcmp(char const *a, char const *b)
{
  while (*a != ' ' && *b != ' ') {
    uint8_t ca = (uint8_t) *a ^ 0x2a;
    uint8_t cb = (uint8_t) *b ^ 0x2a;
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
frobcmpp(char const **a, char const **b)
{
  return frobcmp(*a, *b);
}

static inline char *
get_one_frobbed(void)
{
  char *line = NULL;
  size_t capacity = 0;
  int r = getdelim(&line, &capacity, ' ', stdin);
  if (r > 0) {
    if (line[r - 1] != ' ') {
      line[r] = ' '; /* Destroys NULL at the end */
    }
    return line;
  } else {
    free(line);
    return NULL;
  }
}

static inline size_t
get_all_frobbed(char ***vec)
{
  size_t vec_capacity = 128;
  size_t vec_size = 0;
  *vec = (char **) xrealloc(NULL, vec_capacity * sizeof(char *));
  char *s;
  while ((s = get_one_frobbed())) {
    if (vec_size == vec_capacity) {
      size_t new_cap = vec_capacity * 2;
      *vec = (char **) xrealloc(*vec, new_cap * sizeof(char *));
      vec_capacity = new_cap;
    }
    (*vec)[vec_size++] = s;
  }
  return vec_size;
}

int
main(void)
{
  setvbuf(stdin, NULL, _IOFBF, 0xffffull);
  setvbuf(stdout, NULL, _IOFBF, 0xffffull);

  char **vec = NULL;
  size_t size = get_all_frobbed(&vec);
  qsort(vec, size, sizeof(char *),
        (int (*)(const void *, const void *)) frobcmpp);
  for (size_t i = 0; i < size; i++) {
    size_t s = 0;
    while (vec[i][s] != ' ') s++;
    fwrite(vec[i], s + 1, 1, stdout);
    free(vec[i]);
  }
  fflush(stdout);
  free(vec);
  return 0;
}
