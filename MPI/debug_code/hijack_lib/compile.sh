#!/bin/bash

rm -f libhijack.so libhijack.o
gcc -Wall -fPIC -c -g -O0  *.c
gcc -shared -o libhijack.so -ldl   *.o
