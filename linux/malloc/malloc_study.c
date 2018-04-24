#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct malloc_chunk {

  size_t mchunk_prev_size;  /* Size of previous chunk (if free).  */
  size_t mchunk_size;       /* Size in bytes, including overhead. */

  struct malloc_chunk* fd;         /* double links -- used only if free. */
  struct malloc_chunk* bk;

  /* Only used for large blocks: pointer to next larger size.  */
  struct malloc_chunk* fd_nextsize; /* double links -- used only if free. */
  struct malloc_chunk* bk_nextsize;
};

typedef struct malloc_chunk* mchunkptr;

#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - 2*sizeof(size_t)))

#define PREV_INUSE 0x1
#define prev_inuse(p)       ((p)->mchunk_size & PREV_INUSE)
#define IS_MMAPPED 0x2
#define chunk_is_mmapped(p) ((p)->mchunk_size & IS_MMAPPED)
#define NON_MAIN_ARENA 0x4
#define chunk_main_arena(p) (((p)->mchunk_size & NON_MAIN_ARENA) == 0)
#define set_non_main_arena(p) ((p)->mchunk_size |= NON_MAIN_ARENA)
#define SIZE_BITS (PREV_INUSE | IS_MMAPPED | NON_MAIN_ARENA)

/* Get size, ignoring use bits */
#define chunksize(p) (chunksize_nomask (p) & ~(SIZE_BITS))
#define chunksize_nomask(p) ((p)->mchunk_size)

int main(int argc, char **argv)
{
	int i;
	int size = atoi(argv[1]);
	char *ptr[10];
	ptr[0] = malloc(8);
	for(i = 1; i < 10; i++){
		ptr[i] = malloc(size);
		memset(ptr[i], 0x0, size);
	}
	
	for(i = 0; i < 10; i++){
		mchunkptr chunk = mem2chunk(ptr[i]);
		printf("%p: array[%d].size = %d, inuse=%d\n", ptr[i], i, (int)chunksize(chunk), (int)prev_inuse(chunk));
	}
	printf("free:\n");
	for(i = 1; i < 10; i++){
		free(ptr[i]);
	}

	for(i = 0; i < 10; i++){
		mchunkptr chunk = mem2chunk(ptr[i]);
		printf("%p: array[%d].size = %d, inuse=%d\n", ptr[i], i, (int)chunksize(chunk), (int)prev_inuse(chunk));
	}
	printf("new malloc:\n");
	for(i = 1; i < 4; i++){
		ptr[i] = malloc(size * 2);
		memset(ptr[i], 0x0, size * 2);
	}
	
	for(i = 0; i < 10; i++){
		mchunkptr chunk = mem2chunk(ptr[i]);
		printf("%p: array[%d].size = %d, inuse=%d\n", ptr[i], i, (int)chunksize(chunk), (int)prev_inuse(chunk));
	}

	printf("free:\n");
	for(i = 1; i < 4; i++){
		free(ptr[i]);
	}

	for(i = 0; i < 4; i++){
		mchunkptr chunk = mem2chunk(ptr[i]);
		printf("%p: array[%d].size = %d, inuse=%d\n", ptr[i], i, (int)chunksize(chunk), (int)prev_inuse(chunk));
	}

}