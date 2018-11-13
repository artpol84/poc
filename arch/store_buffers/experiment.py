#!/usr/bin/python

import sys
import os

tmpl_fname = sys.argv[1]
max_stores = int(sys.argv[2])

i = 1
while (i <= max_stores):
    os.system('python generate.py store_tmpl.c test.c ' + str(i))
    os.system('gcc -O3 -o test test.c')
    os.system('./test')
    os.system('rm test*')
    i += 1
