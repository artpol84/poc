#!/bin/bash

exec_it()
{
    tmp=`$1 $2 $3 | grep "Work"`
    echo $2 $tmp
}

exec_it $1 1 1600000
exec_it $1 2 800000
exec_it $1 3 533333
exec_it $1 4 400000
exec_it $1 5 320000
exec_it $1 8 200000
exec_it $1 10 160000
exec_it $1 20 80000
exec_it $1 40 40000
exec_it $1 160 10000 