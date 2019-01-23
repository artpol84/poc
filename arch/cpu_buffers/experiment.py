#!/usr/bin/python

import sys
import os

# Output the command line
cmdline = ""
for i in sys.argv:
    cmdline += i + " "
print cmdline
sys.stdout.flush()

# Output the micro-architecture of the CPU
os.system('gcc -o getarch ../x86/detect_arch/getarch.c')
os.system('./getarch | grep uArch')
os.system('rm getarch')

argv_len = len(sys.argv)
gen_script = sys.argv[1]
max_stores = int(sys.argv[2])

start = 3
if ((argv_len > start) and (sys.argv[start] != '-')) :
    noops = int(sys.argv[3])
    start += 1
else :
    noops = 500

if ((argv_len > start) and (sys.argv[start] != '-')) :
    noops_wait = int(sys.argv[4])
    start += 1
else :
    noops_wait = 10000

remain = ""
if ((argv_len > start) and (sys.argv[start] == '-')) :
    remain = " ".join(map(str, sys.argv[(start + 1):]))

gen_script += "/generate.py"
i = 1
while (i <= max_stores):
    os.system('python ' + gen_script + ' store_tmpl.c test.c ' + str(i) + ' ' + str(noops) + ' ' + str(noops_wait) + ' ' + remain)
    os.system('gcc -O3 -o test -mavx test.c')
    os.system('./test')
    os.system('rm test*')
    i += 1
