#!/usr/bin/python

import sys
import os

gen_script = sys.argv[1]
max_stores = int(sys.argv[2])
noops = int(sys.argv[3])

gen_script += "/generate.py"
i = 1
while (i <= max_stores):
    os.system('python ' + gen_script + ' store_tmpl.c test.c ' + str(i) + ' ' + str(noops))
    os.system('gcc -O3 -o test -mavx test.c')
    os.system('./test')
    os.system('rm test*')
    i += 1
