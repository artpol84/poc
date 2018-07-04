#!/bin/bash

exec_it()
{
    tmp=`$1 $2 $3 | grep "Work"`
    echo $2 $tmp
}

exec_it $1 1  1600000
exec_it $1 2   800000
exec_it $1 4   400000
exec_it $1 8   200000
exec_it $1 16  100000
exec_it $1 32   50000
exec_it $1 64   25000
