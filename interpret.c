#include <stdlib.h>
#include <string.h>

#include "interpret.h"

#include "types.h"
#include "symbols.h"

/*------------------
 * Assertion--------
 */

struct Clause {
  int vars;
  intptr_t *lhs, *rhs;
  struct Clause *next;
};

void __variable_replace(intptr_t *t, List **l, int *elements) {
  int a = functor_arity(*t);
  if (a == -1) {
    /* find the index of the variable */
    int i = 0;
    List *s = *l;
    for (; s; (s = s->next, i++)) {
      if ((intptr_t)s->item == *t) {
	break;
      }
    }
    if (!s) {
      List *this = malloc(sizeof(List));
      this->item = (void *)*t;
      this->next = *l;
      *l = this;
      i = 0;
      (*elements)++;
    }
    i = *elements-i-1;

    /* replace the real variable with a local version */
    *t = 2*i;
  } else {
    /* replace variables in each of the children */
    for (int i = 1; i <= a; i++) {
      __variable_replace((intptr_t *)*(t+i), l, elements);
    }
  }
}

/* replace variables with local versions and return the
 * number of variables
 *
 * Preconditions: t is a term where each argument is a
 *  variable that points directly to the child term
 *  (e.g., the output of our parsing program)
 * Side effects: modifies t 
 */
int variable_replace(intptr_t *t) {
  List *l = malloc(sizeof(List));
  int elements = 0;
  __variable_replace(t, &l, &elements);
  while(l) {
    List *old_l = l;
    l = l->next;
    free(old_l);
  }
  return elements;
}

/* add the clause to the interpretation environment
 *
 * Preconditions: same as variable_replace
 */
void assert(intptr_t *t) {
  int vars = variable_replace(t);
  struct Clause *clause = malloc(sizeof(struct Clause));

  clause->vars = vars;
  clause->next = NULL;

  /* find the predicate, keeping in mind if this is a
     rule or fact */
  intptr_t predicate;
  if (!strcmp(functor_name(*t), ":-") && functor_arity(*t) == 2) {
    predicate = *(intptr_t *)*(t+1);
  } else {
    predicate = *t;
  }

  /* add the clause to its functor's list */
  struct Clause **fdata = (struct Clause **)functor_data(predicate);
  if (*fdata) {
    struct Clause *el;
    for (el = *fdata; el->next; el = el->next) {
      /* skip to the end */
    }
    el->next = clause;
  } else {
    *fdata = clause;
  }

  if (!strcmp(functor_name(*t), ":-") && functor_arity(*t) == 2) {
    /* this is a rule */
    clause->lhs = (intptr_t *)*(t+1);
    clause->rhs = (intptr_t *)*(t+2);
  } else {
    /* this is a fact */
    clause->lhs = t;
    clause->rhs = NULL;
  }
}

/*------------------
 * Interpretation---
 */

/* "trail" of "breadcrumbs" for backtracking */
struct Crumb {
  intptr_t *addr;
  struct Crumb *next;
};

struct Crumb *trail = NULL;

/* insert dummy crumb for choicepoint */
void insert_choicepoint(void) {
  struct Crumb *this = malloc(sizeof(struct Crumb));
  this->addr = NULL;
  this->next = trail;
  trail = this;
}

/* undo until the last choicepoint */
void undo_bindings(void) {
  struct Crumb *old_trail;
  while (trail && trail->addr) {
    *trail->addr = (intptr_t)NULL;
    old_trail = trail;
    trail = trail->next;
    free(old_trail);
  }
  if (trail) {
    old_trail = trail;
    trail = trail->next;
    free(old_trail);
  }
}

/* Find the representative of a term
 *
 * Note: unlike some Prolog implementations that use a
 *  self-pointer to represent an unbound variable, we
 *  use a null-pointer
 */
intptr_t *rep(intptr_t *t) {
  while (*t && *t % 2 == 0) {
    t = (intptr_t *)*t;
  }
  return t;
}

/* Unify arbitrary terms a and b
 *
 * Side effects: modifies variables a and b so they are
 *  equal, adds to trail
 */
int unify(intptr_t *a, intptr_t *b) {
  a = rep(a);
  b = rep(b);
  if (*a == *b) {
    /* Either a and b have the same functor or are the
       same variable */
    int arity = functor_arity(*a);

    /* Unify children (or do nothing for variables) */
    for (int i = 1; i <= arity; i++) {
      if(!unify(a+i, b+i)) {
	return 0;
      }
    }
    return 1;
  } else if (*a % 2 == 0) {
    struct Crumb *this = malloc(sizeof(struct Crumb));
    this->addr = a;
    this->next = trail;
    trail = this;
    *a = (intptr_t)b;
    return 1;
  } else if (*b % 2 == 0) {
    struct Crumb *this = malloc(sizeof(struct Crumb));
    this->addr = b;
    this->next = trail;
    trail = this;
    *b = (intptr_t)a;
    return 1;
  } 
  /* Failure */
  return 0;
}

/* copies template with the variables requested
 *
 * Preconditions: template is represented as returned by
 *  variable_replace
 */
intptr_t *copy_template(intptr_t *template,
			intptr_t **vars) {
  intptr_t *copy;
  if (*template % 2) {
    int arity = functor_arity(*template);
    copy = malloc(sizeof(intptr_t)*(arity+1));
    copy[0] = *template;
    for (int i = 1; i <= arity; i++) {
      copy[i] =
	(intptr_t)copy_template((intptr_t *)*(template+i), vars);
    }
  } else {
    copy = malloc(sizeof(intptr_t));
    *copy = (intptr_t)(vars+*template/2);
  }
  return copy;
}

/* Match the template with the term, unifying as
 * necessary
 *
 * Preconditions: template is represented as returned by
 *  variable_replace, vars contains enough space for
 *  all variables in template
 * Side effects: modifies t to unify with the template
 */
int match(intptr_t *template, intptr_t *t,
	  intptr_t **vars) {
  t = rep(t);
  if (*template % 2) {
    /* template is a term */
    if (*t % 2) {
      /* t is also a term */
      if (*template == *t) {
	/* same functor, so match children */
	int arity = functor_arity(*t);
	for (int i = 1; i <= arity; i++) {
	  if (!match((intptr_t *)*(template+i),
		     (intptr_t *)*(t+i),
		     vars)) {
	    return 0;
	  }
	}
	return 1;
      } else {
	/* functor is different, so this is a failure */
	return 0;
      }
    } else {
      /* copy the template into t */
      *t = (intptr_t)copy_template(template, vars);
      return 1;
    }
  } else {
    intptr_t **var = vars+*template/2;
    if (*var) {
      /* the variable is set already, so unify */
      return unify(*var, t);
    } else {
      /* set the variable */
      *var = t;
      return 1;
    }
  }
}

/* the 'and' functor */
/* intptr_t and_f; */

/* Execute the call (represented as a term)
 *
 * Side effects: performs unification on call, but
 *  undoes it in the case of failure
 */
int execute(intptr_t *call) {
  struct Clause *c = *(struct Clause **)functor_data(*call);
  for (; c; c = c->next) {
    intptr_t **vars =
      calloc(c->vars, sizeof(intptr_t *));

    /* match up the head and the predicate call */
    insert_choicepoint();
    if (!match(c->lhs, call, vars)) {
      undo_bindings();
      continue;
    }

    if (c->rhs) {
      /* execute the body */
      intptr_t *body = copy_template(c->rhs, vars);
      if (!execute(body)) {
	undo_bindings();
	continue;
      }
      return 1;
    } else {
      /* return true since this is a fact */
      return 1;
    }
  }

  /* no success clause was found, so fail */
  return 0;
}

/* Executes the main predicate */
int execute_main(void) {
  /* and_f = symbol_index("and", 2); */
  intptr_t main_symbol = symbol_index("main", 0);
  return execute(&main_symbol);
}
