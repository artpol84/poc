#include <stdio.h>

__thread int tid = 0;
int gid = 0;


int func_tloc()
{
	return (tid++);
}

int func_glob()
{
	return (gid++);
}

int func_floc()
{
	int i = 0;
	return (i++);
}

int func_fstatic()
{
	static int i = 0;
	return (i++);
}
