#ifndef TYPES_H
#define TYPES_H

typedef struct List {
  void *item;
  struct List *next;
} List;

#endif
