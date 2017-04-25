/* -*- mode: c; c-basic-offset: 2; -*- */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

static void
parse_args(int argc, const char **argv, uint8_t *translate_table)
{
  if (argc != 3) {
    fprintf(stderr, "%s: invalid number of operands; expected 2, actual %d\n",
            argv[0], argc - 1);
    exit(1);
  }

  size_t len1 = strlen(argv[1]), len2 = strlen(argv[2]);
  if (len1 != len2) {
    fprintf(stderr,
            "%s: FROM and TO strings must be of the same length; "
            "actual lengths: %zu and %zu\n",
            argv[0], len1, len2);
    exit(1);
  }

  uint8_t dups[256];
  memset(dups, 0, sizeof dups);

  for (int i = 0; i < 256; ++i) translate_table[i] = i;

  for (size_t i = 0; i < len1; ++i) {
    if (dups[(uint8_t) argv[1][i]]) {
      fprintf(stderr, "%s: duplicate character %c found in FROM string\n",
              argv[0], argv[1][i]);
      exit(1);
    }
    dups[(uint8_t) argv[1][i]] = 1;
    translate_table[(uint8_t) argv[1][i]] = argv[2][i];
  }
}

#ifndef TR2U_BUFSIZE
#define TR2U_BUFSIZE 1
#endif

int
main(int argc, const char **argv)
{
  uint8_t translate_table[256];
  parse_args(argc, argv, translate_table);

  for (;;) {
    uint8_t buf[TR2U_BUFSIZE];
    ssize_t r = read(STDIN_FILENO, buf, sizeof buf);
    if (r == 0)
      break;
    else if (r == -1) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      else {
        fprintf(stderr, "%s: could not read stdin: %s\n", argv[0],
                strerror(errno));
        exit(1);
      }
    } else {
      for (int i = 0; i < r; i++) buf[i] = translate_table[buf[i]];
      ssize_t w = write(STDOUT_FILENO, buf, r);
      if (w != r) {
        fprintf(stderr, "%s: could not write stdout: %s\n", argv[0],
                strerror(errno));
      }
    }
  }

  return 0;
}
