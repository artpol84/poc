#include <stdio.h>

__thread int id = 0;

int func()
{
	return (id++);
}