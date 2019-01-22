#!/usr/bin/python

import sys
import os


argv_len = len(sys.argv)
gen_script = sys.argv[1]
max_stores = int(sys.argv[2])
if (argv_len > 3) :
    noops = int(sys.argv[3])
else :
    noops = 500

if (argv_len > 4) :
    noops_wait = int(sys.argv[4])
else :
    noops_wait = 10000

gen_script += "/generate.py"
i = 1
while (i <= max_stores):
    os.system('python ' + gen_script + ' store_tmpl.c test.c ' + str(i) + ' ' + str(noops) + ' ' + str(noops_wait))
    os.system('gcc -O3 -o test -mavx test.c')
    os.system('./test')
    os.system('rm test*')
    i += 1
