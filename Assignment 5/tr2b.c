/* -*- mode: c; c-basic-offset: 2; -*- */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

int
main(int argc, const char **argv)
{
  uint8_t translate_table[256];
  parse_args(argc, argv, translate_table);
  int ch;
  while ((ch = getchar()) != EOF) putchar(translate_table[ch]);
  return 0;
}
