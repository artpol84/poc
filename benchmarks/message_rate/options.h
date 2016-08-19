#ifndef OPTIONS_H
#define  OPTIONS_H

#include <stdio.h>
#include "mr_th_nb.h"

void set_default_args(settings_t *s);
void pre_scan_args(int argc, char **argv, settings_t *s);
int process_args(int argc, char **argv, settings_t *s);

#endif