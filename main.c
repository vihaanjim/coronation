#include <stdio.h>

#include "parse.h"
#include "symbols.h"
#include "interpret.h"

/* void debug_assert(intptr_t *t) { */
  /* print_term(t); */
  /* puts("."); */
/* } */

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("Error: no file specified", stderr);
    return 1;
  }

  FILE *f;
  f = fopen(argv[1], "r");

  parse(f, &assert);
  return !execute_main();
}
