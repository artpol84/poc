#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

__thread int tid = 0;
int gid = 0;

static pthread_key_t key;

static int th_id = 0;

typedef struct key_test {
        int val;
        int id;
} key_test_t;

void pth_key_cleanup(void *arg) {
        key_test_t *ptr = (key_test_t *)arg;
        (void)ptr;
        // printf("cleanup: id = %d\n", ptr->id);
        free(arg);
}

void lib_init()
{
        (void) pthread_key_create(&key, pth_key_cleanup);
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
        key_test_t *ptr = (key_test_t *)pthread_getspecific(key);

        if (unlikely(ptr == NULL)) {
            ptr = calloc(1, sizeof(key_test_t));
            (void) pthread_setspecific(key, ptr);
            ptr->id = th_id++;
            ptr->val = 0;
        }

        return (ptr->val++);
}
