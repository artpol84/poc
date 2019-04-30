#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#define xmalloc malloc
#define xfree free

typedef struct {
	uint32_t nnodes; /* number of nodes in this namespace */
	uint32_t ntasks; /* total number of tasks in this namespace */
	uint32_t *task_cnts; /* Number of tasks on each node of namespace */
	uint32_t *task_map; /* i'th task is located on task_map[i] node */
} pmixp_namespace_t;


int pmixp_count_digits_base10(uint32_t val)
{
	int digit_count = 0;
	while(val) {
		digit_count++;
		val /= 10;
	}
	return digit_count;
}

/* Build a sequence of ranks sorted by nodes */
static void _build_node2task_map(pmixp_namespace_t *nsptr, uint32_t *node2tasks)
{
	int i;
	uint32_t *node_offsets = xmalloc(nsptr->nnodes * sizeof(*node_offsets));
	uint32_t *task_cnts_cur = xmalloc(nsptr->nnodes * sizeof(*task_cnts_cur));
	/* Build the offsets structure needed to fill the node-to-tasks map */
	for(i = 1; i < nsptr->nnodes; i++) {
		node_offsets[i] = node_offsets[i-1] + nsptr->task_cnts[i-1];
	}

	/* Fill the node-to-task map */
	for(i = 0; i < nsptr->ntasks; i++) {
		int node = nsptr->task_map[i];
		int offset = node_offsets[node] + task_cnts_cur[node]++;
		node2tasks[offset] = i;
	}
	/* Cleanup service structures */
	xfree(node_offsets);
	xfree(task_cnts_cur);
}

/* Estimete the size of a buffer capable of holding the proc map for this job.
 * PMIx proc map string format:
 *
 *    xx,yy,...,zz;nn,mm,..ll;...;aa,bb,...cc;
 *    --n0 ranks--;--n1------;...;----nX-----;
 *
 * To roughly estimate the size of the string we leverage the following
 * dependency: for any rank \in [0; nspace->ntasks - 1]
 *     num_digits_10(rank) <= num_digits_10(nspace->ntasks)
 *
* So we can say that the cumulative number "digits_cnt" of all symbols
* comprising all rank numbers in the namespace is:
*     digits_size <= num_digits_10(nspace->ntasks) * nspace->ntasks
* We roughly assume that every rank is followed by a comma (not quite
* true as ";" replaces "," on node borders.
* We also assume that number of ";" symbols is equal to number of nodes
* "+1" added to account the '\0' symbol
* The final formula is:
*    (num_digits_10(nspace->ntasks) + 1) * nspace->ntasks + nnodes
*
* Considering a 1.000.000 core system with 64PPN.
* The size of the intermediate buffer will be:
* - num_digits_10(1.000.000) = 7
* - (7 + 1) * 1.000.000 + (1.000.000)/64 ~ 8MB
* This is a reasonable overhead for such a big system.
* Also note that this is a temp buffer that will be released
*/
static size_t _proc_map_buffer_size(pmixp_namespace_t *nsptr)
{
	return ((pmixp_count_digits_base10(nsptr->ntasks) + 1) * nsptr->ntasks
		+ nsptr->nnodes + 1);
}

#define _PMIXP_LOC_STRCAT(outstr, remain, ret, err_label, fmt, args...) {  \
	ret = snprintf(outstr,remain, fmt, ## args );                      \
	outstr += ret;                                                     \
	remain -= ret;                                                     \
	if( remain < 0 ){                                                  \
		goto err_label;                                            \
	}                                                                  \
}

static char *_build_procmap(pmixp_namespace_t *nsptr)
{
	/* 1. Count number of digits in the numtasks */
	size_t str_size = _proc_map_buffer_size(nsptr);
	ssize_t remain = 0, ret = 0;
	uint32_t *node2tasks = xmalloc(nsptr->ntasks * sizeof(*node2tasks));
	uint32_t *cur_task = NULL;
	char *output = xmalloc(str_size);
	char *ptr = NULL;
	int i, j;

	/* Build a node-to-tasks map that is more convenient to traverse */
	_build_node2task_map(nsptr, node2tasks);

	ptr = output;
	remain = str_size;
	cur_task = node2tasks;
	for (i = 0; i < nsptr->nnodes; i++) {
		int first = 1;
		/* cur_task is expected to point to the first rank on the node
		 * and all node ranks are placed sequentially
		 * So all we need to do is to traverse nsptr->task_cnts[i]
		 * sequential elements starting from cur_task
		 */
		for (j = 0; j < nsptr->task_cnts[i]; j++) {
			if (first) {
				first = 0;
			} else {
				_PMIXP_LOC_STRCAT(ptr, remain, ret,
						  err_exit, ",")
			}
			_PMIXP_LOC_STRCAT(ptr, remain, ret, err_exit,
					  "%u", *(cur_task++));
		}
		/* Put the separator if not the last node */
		if (i < (nsptr->nnodes - 1)) {
			_PMIXP_LOC_STRCAT(ptr, remain, ret, err_exit, ";");
		}
	}
	/* Cleanup service structures */
	xfree(node2tasks);
	return output;
err_exit:
	if (node2tasks) {
		xfree(node2tasks);
	}
	if(output) {
		xfree(output);
	}
	return NULL;
}

static char *_build_procmap_old(pmixp_namespace_t *nsptr)
{
	/* 1. Count number of digits in the numtasks */
	size_t str_size = _proc_map_buffer_size(nsptr);
	ssize_t remain = 0, ret = 0;
	uint32_t *node2tasks = xmalloc(nsptr->ntasks * sizeof(*node2tasks));
	uint32_t *cur_task = NULL;
	char *output = xmalloc(str_size);
	char *ptr = NULL;
	int i, j;

	/* Build a node-to-tasks map that is more convenient to traverse */
	_build_node2task_map(nsptr, node2tasks);

	ptr = output;
	remain = str_size;
	cur_task = node2tasks;
	for (i = 0; i < nsptr->nnodes; i++) {
		int first = 1;
		/* cur_task is expected to point to the first rank on the node
		 * and all node ranks are placed sequentially
		 * So all we need to do is to traverse nsptr->task_cnts[i]
		 * sequential elements starting from cur_task
		 */
		for (j = 0; j < nsptr->ntasks; j++) {
			if( nsptr->task_map[j] == i ){
				if (first) {
					first = 0;
				} else {
					_PMIXP_LOC_STRCAT(ptr, remain, ret,
							  err_exit, ",")
				}
				_PMIXP_LOC_STRCAT(ptr, remain, ret, err_exit,
						  "%u", j);
			}
		}
		/* Put the separator if not the last node */
		if (i < (nsptr->nnodes - 1)) {
			_PMIXP_LOC_STRCAT(ptr, remain, ret, err_exit, ";");
		}
	}
	/* Cleanup service structures */
	xfree(node2tasks);
	return output;
err_exit:
	if (node2tasks) {
		xfree(node2tasks);
	}
	if(output) {
		xfree(output);
	}
	return NULL;
}


int main(int argc, char **argv)
{
	pmixp_namespace_t *nsptr = xmalloc(sizeof(nsptr));
	int mapping = 2;
	int i, j;
	int ppn = 40;
	int use_old = 0;
	if( argc < 2 ){
		printf("Need node count!\n");
		return 0;
	}
	nsptr->nnodes = atoi(argv[1]);
	if( argc > 2 ) {
		use_old = 1;
	}
	nsptr->ntasks = nsptr->nnodes * ppn;
	nsptr->task_cnts = xmalloc(sizeof(int) * nsptr->nnodes);
	nsptr->task_map = xmalloc(sizeof(*nsptr->task_map) * nsptr->ntasks);
	for(i=0; i<nsptr->nnodes; i++) {
		nsptr->task_cnts[i] = ppn;
	}

	if( mapping == 1 /*by core */) {
		int task_id = 0;
		for(i=0; i<nsptr->nnodes; i++) {
			for(j=0; j<ppn; j++) {
				nsptr->task_map[task_id++] = i;
			}
		}
	} else if( mapping == 2 /*by node */) {
		int node_id = 0;
		for(i=0; i<nsptr->ntasks; i++) {
			nsptr->task_map[i] = node_id;
			node_id = (node_id + 1) % nsptr->nnodes;
		}
	}

	char *output = NULL;
	if( use_old ) {
		output = _build_procmap_old(nsptr);
	} else {
		output = _build_procmap(nsptr);
	}
//	printf("output = %s\n", output);
	xfree(output);
}
