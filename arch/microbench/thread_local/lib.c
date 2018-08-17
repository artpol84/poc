#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

__thread int tid = 0;
int gid = 0;

static pthread_key_t key;

void lib_init()
{
        (void) pthread_key_create(&key, NULL);
}

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

int func_pth_key()
{
        int *ptr = (int *)pthread_getspecific(key);

        if (unlikely(ptr == NULL)) {
            ptr = calloc(1, sizeof(int));
            (void) pthread_setspecific(key, ptr);
        }

        return ((*ptr)++);
}
