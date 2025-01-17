#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "symbols.h"

struct FunctorPair {
  char *name;
  char arity;
  void *data;
};

struct FunctorPair *table = NULL;
int table_length = 0;
int table_capacity = 8;

intptr_t symbol_index(char *name, char arity) {
  if (!table) {
    table = malloc(8*sizeof(struct FunctorPair));
  }
  if (isupper(name[0])) {
    arity = -1;
  }
  for (int i = 0; i < table_length; i++) {
    if (!strcmp(table[i].name, name) && table[i].arity == arity) {
      return 2*i+1;
    }
  }

  if (table_length == table_capacity) {
    table = realloc(table, sizeof(struct FunctorPair)*(table_capacity *= 2));
  }

  // note: this function is only called on names that
  // are heap-allocated
  table[table_length].name = name;
  table[table_length].arity = arity;
  table[table_length].data = NULL;

  return 2*(table_length++) + 1;
}

char *functor_name(intptr_t functor) {
  int true_functor = (functor - 1) / 2;
  if (table && true_functor < table_length) {
    return table[true_functor].name;
  } else {
    return NULL;
  }
}

int functor_arity(intptr_t functor) {
  int true_functor = (functor - 1) / 2;
  if (table && true_functor < table_length) {
    return table[true_functor].arity;
  } else {
    return -0xdead;
  }
}

void **functor_data(intptr_t functor) {
  int true_functor = (functor - 1) / 2;
  if (table && true_functor < table_length) {
    return &table[true_functor].data;
  } else {
    return NULL;
  }
}

void print_term(intptr_t *p) {
  if (!(*p % 2)) {
    if ((intptr_t *)*p == p) {
      printf("%p", (void *)p);
    } else {
      print_term((intptr_t *)*p);
    }
  } else {
    printf("%s", functor_name(*p));
    int a = functor_arity(*p);
    if (a > 0) {
      putc('(', stdout);
      for (int i = 1; i <= a; i++) {
	print_term(p+i);
	if (i != a) {
	  putc(',', stdout);
	  putc(' ', stdout);
	}
      }
      putc(')', stdout);
    }
  }
}
