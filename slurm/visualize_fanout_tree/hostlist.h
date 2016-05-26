#ifndef HOSTLIST_H
#define HOSTLIST_H
#include <stdio.h>

char *skip_spaces(char *ptr);
char *find_next(char *ptr);
int hostlist_count(char *hl);
char *hostlist_shift(char *hl, char **name);

#endif