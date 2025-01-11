#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>
#include <stdint.h>

void parse(FILE *f, void (*assert)(intptr_t *t));

#endif
