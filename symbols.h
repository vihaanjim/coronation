#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdint.h>
/* #include "types.h" */

intptr_t symbol_index(char *name, char arity);

char *functor_name(intptr_t functor);

int functor_arity(intptr_t functor);

void print_term(intptr_t *p);

#endif
