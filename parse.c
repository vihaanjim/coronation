#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "types.h"
#include "symbols.h"

void parse(FILE *f, void (*assert)(intptr_t *t)) {
  List *stack = NULL;

  char functor[32];
  unsigned char f_ind = 0;

  int c;
  while ((c = fgetc(f)) != EOF) {
    // first, add atoms if necessary
    switch (c) {
    case ')':
    case ',':
    case ' ':
    case '\n':
      if (f_ind) {
	// copy the name of the functor
	char *f_name = malloc((f_ind+1)*sizeof(char));
	memcpy(f_name, functor, f_ind);
	f_name[f_ind] = 0;
	
	// create the new term
	intptr_t *t = malloc(sizeof(intptr_t));
	*t = symbol_index(f_name, 0);

	// add it to the stack
	List *this = malloc(sizeof(List));
	this->item = t;
	this->next = stack;
	stack = this;

	// reset the functor index
	f_ind = 0;
      }
      break;
    }

    // now, process
    switch (c) {
    case '(':
      if (f_ind) {
	// copy the name of the functor
	char *f_name = malloc((f_ind+1)*sizeof(char));
	memcpy(f_name, functor, f_ind);
	f_name[f_ind] = 0;

	// add the functor and a separator to the stack
	List *this = malloc(sizeof(List));
	this->item = (void *)f_name;
	this->next = stack;

	List *sep = malloc(sizeof(List));
	sep->item = NULL;
	sep->next = this;

	stack = sep;

	// reset the functor index
	f_ind = 0;
      }
      break;
    case ')':
      {
	// simultaneously count the arity, rearrange the terms, and remove them from the stack
	int arity = 0;
	List *args = NULL;
	while (1) {
	  if (stack->item) {
	    // remove the top item from the stack
	    List *cur = stack;
	    stack = stack->next;

	    // add it to the list of args
	    cur->next = args;
	    args = cur;

	    // increment the arity
	    arity++;
	  }
	  else {
	    // remove the separator
	    stack = stack->next;
	    break;
	  }
	}

	// create the new term
	intptr_t *t = malloc(sizeof(intptr_t)*(arity+1));
	*t = symbol_index((char *)stack->item, arity);
	for (List *l = args; l; (l = l->next, t++)) {
	  *(t+1) = (intptr_t)l->item;
	}

	// replace the functor at the top of the stack
	stack->item = t-arity;
	
	break;
      }
    case '.':
      assert((intptr_t *)stack->item);
      break;
    case ',':
    case ' ':
    case '\n':
      break; 
    default:
      functor[f_ind++] = c;
      break;
    }
  }
}
