#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>

#include "platform.h"

static size_t flush_array_sz = 0;
static char *flush_array = NULL;

size_t 
cache_line_size()
{
	FILE * p = 0;
	p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
	unsigned int i = 0;
	if (p) {
		fscanf(p, "%d", &i);
		fclose(p);
	}
	return i;
}

static int topo_init(topo_info_t *info)
{
    /* Allocate and initialize topology object. */
    if (hwloc_topology_init(&info->topology)){
        printf("hwloc: hwloc_topology_init() failed\n");
        return -1;
    }
 
    /* ... Optionally, put detection configuration here to ignore
       some objects types, define a synthetic topology, etc....
 
       The default is to detect all the objects of the machine that
       the caller is allowed to access.  See Configure Topology
       Detection. */
 
    /* Perform the topology detection. */
    if (hwloc_topology_load(info->topology) ){
        printf("hwloc: hwloc_topology_load() failed\n");
        return -1;
    }
 
    /* Optionally, get some additional topology information
       in case we need the topology depth later. */
    info->depth = hwloc_topology_get_depth(info->topology);

    return 0;
}

static int topo_discover_one_cache(topo_info_t *info, int level_id, 
                            hwloc_obj_t obj, topo_cache_t *cache)
{
    cache->idx = obj->logical_index;
    cache->refcnt = 0;
    cache->size = obj->attr->cache.size;
    cache->level_id = level_id;
    return 0;
}

static int topo_discover_cache_level(topo_info_t *info, int depth, int level_id,
                                topo_cache_level_t *lvl)
{
    hwloc_obj_t obj;
    int i;

    lvl->level_id = level_id;
    lvl->depth = depth;
    lvl->ncaches = hwloc_get_nbobjs_by_depth(info->topology, depth);
    lvl->caches = calloc(lvl->ncaches, sizeof(lvl->caches[0]));

    for(i = 0; i < lvl->ncaches; i++) {
        obj = hwloc_get_obj_by_depth(info->topology, depth, i);
        topo_discover_one_cache(info, level_id, obj, &lvl->caches[i]);
        assert(i == lvl->caches[i].idx);
    }
    return 0;
} 

static int topo_discover_caches(topo_info_t *info)
{
    topo_cache_subs_t *cs = &info->cache_subs;
    hwloc_obj_t obj;
    int depth = hwloc_get_type_depth(info->topology, HWLOC_OBJ_CORE);
    int i;

    /* initialize the memory */
    memset(cs, 0, sizeof(*cs));

    cs->nlevels = 0;
    /* Traverse levels bottom-up marking caches */
    for(i = depth; i > 0; i--) {
        obj = hwloc_get_obj_by_depth(info->topology, i, 0);
        if (hwloc_obj_type_is_cache(obj->type)) {
            if (topo_discover_cache_level(info, i, cs->nlevels, 
                                            &cs->level[cs->nlevels])) {
                return -1;
            }
            cs->nlevels++;
        }
    }

    return 0;
}

static topo_cache_t *topo_cache_find(topo_info_t *info,  hwloc_obj_t obj)
{
    topo_cache_subs_t *cs = &info->cache_subs;
    int i;

    for(i = 0; i < cs->nlevels; i++){
        if (obj->depth == cs->level[i].depth) {
            assert(obj->logical_index < cs->level[i].ncaches);
            return &cs->level[i].caches[obj->logical_index];
        }
    }
    return NULL;
}

static int topo_discover_cores(topo_info_t *info)
{
    topo_core_subs_t *cinfo = &info->core_subs;
    topo_cache_subs_t *cs = &info->cache_subs;
    topo_core_t *core;
    hwloc_obj_t obj;
    int i, j;

    cinfo->depth = hwloc_get_type_depth(info->topology, HWLOC_OBJ_CORE);
    if (cinfo->depth == HWLOC_TYPE_DEPTH_UNKNOWN) {
        printf("hwloc: Failed to detect cores\n");
        return -1;
    }
    cinfo->count = hwloc_get_nbobjs_by_depth(info->topology, cinfo->depth);
    cinfo->cores = calloc(cinfo->count, sizeof(cinfo->cores[0]));

    /* Iterate over cores */
    for (i = 0; i < cinfo->count; i++) {
        core = &cinfo->cores[i];
        obj = hwloc_get_obj_by_depth(info->topology, cinfo->depth, i);
        /* 1. init core info */
        core->core_id = obj->logical_index;
        core->cpuset = hwloc_bitmap_dup(obj->cpuset);
        /* 2. go up the hierarchy to analyze caches */
        for (; obj; obj = obj->parent) {
            if (hwloc_obj_type_is_cache(obj->type)) {
                topo_cache_t *c = topo_cache_find(info, obj);
                c->refcnt++;
                core->cache_refs[c->level_id].valid = 1;
                core->cache_refs[c->level_id].ref = c; 
            }
        }
    }

    /* Allocate core IDs for caches */
    for(i = 0; i < cs->nlevels; i++) {
        for(j = 0; j < cs->level[i].ncaches; j++) {
            cs->level[i].caches[j].refs = calloc(cs->level[i].caches[j].refcnt,
                                                sizeof(cs->level[i].caches[j].refs[0]));
            cs->level[i].caches[j].refs_inuse = 0;
        }
    }

    /* Finish up the topology building */
    for (i = 0; i < cinfo->count; i++) {
        core = &cinfo->cores[i];
        for (j = 0; j < cs->nlevels; j++) {
            topo_core_cache_ref_t *cref = &core->cache_refs[j];
            if(cref->valid) {
                cref->shrd_frac = cref->ref->size/cref->ref->refcnt;
                cref->ref->refs[cref->ref->refs_inuse++] = core;
            }
        }
    }
    
    return 0;
}

void topo_print_cores(topo_info_t *info)
{
    topo_core_subs_t *cinfo = &info->core_subs;
    topo_cache_subs_t *cs = &info->cache_subs;
    int i, j;

    /* Output core info */
    for (i = 0; i < cinfo->count; i++) {
        topo_core_t *core = &cinfo->cores[i];
        printf("core#%d: ", core->core_id);
        
        for (j = 0; j < cs->nlevels; j++) {
            if(core->cache_refs[j].valid) {
                printf("[L%d sz=%zd refcnt=%d, frac=%zd]\n",
                        core->cache_refs[j].ref->level_id,
                        core->cache_refs[j].ref->size,
                        core->cache_refs[j].ref->refcnt,
                        core->cache_refs[j].shrd_frac);
            }
        }
    }
}

void topo_print_caches(topo_info_t *info)
{
    topo_cache_subs_t *cs = &info->cache_subs;
    int i, j, k;

    /* Output cache info */
    
    printf("Cache subsystem details:\n");
    printf("\t# of levels: %d\n", cs->nlevels);
    for(i = 0; i < cs->nlevels; i++) {
        printf("\tL%d\n", cs->level[i].level_id);
        for(j = 0; j < cs->level[i].ncaches; j++) {
            printf("\t\tIdx%d (%d): ", cs->level[i].caches[j].idx,
                                   cs->level[i].caches[j].refs_inuse);
            for(k = 0; k < cs->level[i].caches[j].refs_inuse; k++) {
                printf("%d ", cs->level[i].caches[j].refs[k]->core_id);
            }
            printf("\n");
        }
    }
}

int caches_discover(cache_struct_t *cache)
{
    topo_cache_subs_t *cs = &cache->topo.cache_subs;
   int i, j;

    cache->cl_size = cache_line_size();

    topo_init(&cache->topo);
    topo_discover_caches(&cache->topo);
    topo_discover_cores(&cache->topo);

    cache->nlevels = cache->topo.cache_subs.nlevels;

    for(i = 0; i < cs->nlevels; i++) {
        cache->cache_sizes[i] = cache->topo.cache_subs.level[i].caches[0].size;
    }

    if (!cache->nlevels) {
         return 0;
    }

    /* Add memory into hirarchy */
    cache->cache_sizes[cache->nlevels] = cache->cache_sizes[cache->nlevels - 1] * 8;
    flush_array_sz = cache->cache_sizes[cache->nlevels];
    cache->nlevels++;

    flush_array = calloc(flush_array_sz, 1);

    return 0;
}

int caches_detect_level(cache_struct_t *cache, size_t buf_size)
{
    int i;
    for(i = 0; i < cache->nlevels; i++) {
        if( buf_size <= cache->cache_sizes[i]) {
            return i;
        }
    }

    /* RAM */
    return cache->nlevels - 1;
}


int caches_finalize(cache_struct_t *cache)
{
    topo_cache_subs_t *cs = &cache->topo.cache_subs;
    int i, j;

    /* Output cache info */
    
    for(i = 0; i < cs->nlevels; i++) {
        for(j = 0; j < cs->level[i].ncaches; j++) {
            free(cs->level[i].caches[j].refs);
        }
        free(cs->level[i].caches);
    }

    /* Destroy topology object. */
    hwloc_topology_destroy(cache->topo.topology);
 
    return 0;
}

void caches_set_default(cache_struct_t *cache)
{
    cache->cl_size = 64;
    cache->nlevels = 4;

    cache->cache_sizes[0] = 32*1024;
    cache->cache_sizes[1] = 1024*1024;
    cache->cache_sizes[2] = 32*1024*1024;
    cache->cache_sizes[3] = cache->cache_sizes[2] * 8;
}

void caches_output(cache_struct_t *cache, int verbose)
{
    int level;

    printf("Cache subsystem (CL size = %zd):\n", cache_line_size());
    for(level=0; level < cache->nlevels; level++) {
        printf("Level: %d\tSize: %zd\n", level + 1, cache->cache_sizes[level]);
    }
    if (verbose) {
        topo_print_cores(&cache->topo);
        topo_print_caches(&cache->topo);
    }

}

void flush_cache()
{
    int i;
    static int count = 0;

    count++;
    for(i=0; i< flush_array_sz; i++){
        flush_array[i] = count;
    }
}
