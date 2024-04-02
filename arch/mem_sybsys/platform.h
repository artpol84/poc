#ifndef MEM_SYS_H
#define MEM_SYS_H

#include <sys/types.h>

#define MEMSUBS_CACHE_LEVEL_MAX 32

#include <hwloc.h> 

typedef struct topo_core_s topo_core_t;
typedef struct topo_info_s topo_info_t;

typedef struct {
    int level_id;
    int idx;
    size_t size;
    int refcnt;
    topo_core_t **refs;
    int refs_inuse;
} topo_cache_t;

typedef struct {
    int depth;
    int ncaches;
    int level_id;
    topo_cache_t *caches;
} topo_cache_level_t;

typedef struct {
    int nlevels;
    topo_cache_level_t level[MEMSUBS_CACHE_LEVEL_MAX];
} topo_cache_subs_t;

typedef struct {
        int valid;
        topo_cache_t *ref;
        size_t shrd_frac;
} topo_core_cache_ref_t;

typedef struct topo_core_s {
    int core_id;
    topo_info_t *topo;
    topo_core_cache_ref_t cache_refs[MEMSUBS_CACHE_LEVEL_MAX];
    hwloc_cpuset_t cpuset;
} topo_core_t;

typedef struct {
    int depth;
    int count;
    topo_core_t *cores;
} topo_core_subs_t;

typedef struct topo_info_s {
    hwloc_topology_t topology;
    int depth;
    topo_cache_subs_t cache_subs;
    topo_core_subs_t core_subs;
} topo_info_t;

typedef struct {
    int nlevels;
    size_t cl_size;
    size_t cache_sizes[MEMSUBS_CACHE_LEVEL_MAX];
    topo_info_t topo;
} cache_struct_t;

int caches_discover(cache_struct_t *cache);
int caches_finalize(cache_struct_t *cache);
void caches_output(cache_struct_t *cache, int verbose);
int caches_detect_level(cache_struct_t *cache, size_t buf_size);
void flush_cache();

#endif

