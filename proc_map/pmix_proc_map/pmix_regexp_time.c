#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <pmix.h>
#include <pmix_server.h>

#include <time.h>

#define ITERS 10
#define VERBOSE 0

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

static char straddfmt(char **str, const char *fmt, ...)
{
    char *p = NULL;
    va_list ap;

    va_start(ap, fmt);
    vasprintf(&p, fmt, ap);
    va_end(ap);

    if (*str == NULL) {
         *str = strdup(p);
        free(p);
        return strlen(*str);
    }

    *str = realloc(*str, strlen(*str) + strlen(p) + 1);
    strcat(*str, p);

    free(p);
    return strlen(*str);
}

int time_map_regex(uint32_t nnodes, uint32_t npernode, char *nname_fmt)
{
    char *node_list = NULL;
    char *map = NULL;
    double ts, nmap_ts = 0.0, pmap_ts = 0.0;
    uint32_t i, j;
    uint32_t cur_task = 0;
    int rc;
    char *regexp;

    /* generate hostlist */
    for (i = 0; i < nnodes; i++) {
        straddfmt(&node_list, nname_fmt, i);
        if (i < (nnodes - 1)) {
            straddfmt(&node_list, ",");
        }
    }
    //printf("node_list %s\n", node_list);

    /* generate map */
    for (i = 0; i < nnodes; i++) {
        char *sep = "";
        for (j = 0; j < npernode; j++){
            straddfmt(&map, "%s%u", sep, cur_task++);
            sep = ",";
        }
        if (i < (nnodes - 1)) {
            straddfmt(&map, ";");
        }
    }
    //printf("map %s\n", map);

    /* generate PMIX_NODE_MAP */
    for (i = 0; i < ITERS; i++) {
        ts = GET_TS();
        rc = PMIx_generate_regex(node_list, &regexp);
        nmap_ts += GET_TS() - ts;
        if (rc != PMIX_SUCCESS) {
            fprintf(stderr, "PMIx_generate_regex failed with error %d\n", rc);
            return 0;
        }
        free(regexp);
    }
    nmap_ts /= ITERS;

    /* generate PMIX_PROC_MAP */
    for (i = 0; i < ITERS; i++) {
        ts = GET_TS();
        rc = PMIx_generate_ppn(map, &regexp);
        pmap_ts += GET_TS() - ts;
        if (rc != PMIX_SUCCESS) {
            fprintf(stderr, "PMIx_generate_ppn failed with error %d\n", rc);
            return 0;
        }
        free(regexp);
    }
    pmap_ts /= ITERS;

    printf("%-10lf %-10lf\n", nmap_ts, pmap_ts);

    free(node_list);
    free(map);
    
    return 0;
}

int main() {
    uint32_t nnodes, i;
    uint32_t npernode = 32;
    char *nname_fmt = "node%d";
    int rc;

    if (PMIX_SUCCESS != (rc = PMIx_server_init(NULL, NULL, 0))) {
        fprintf(stderr, "Init failed with error %s\n", PMIx_Error_string(rc));
        return rc;
    }

    printf("%-10s %-10s %-10s %-10s\n",
           "nnodes", "procs", PMIX_NODE_MAP, PMIX_PROC_MAP);
    printf("-----------------------------------------\n");

    for (i = 0; i < 16; i++) {
        nnodes = 1 << i;
        printf("%-10u %-10u ", nnodes, nnodes * npernode);
        time_map_regex(nnodes, npernode, nname_fmt);
    }

    if (PMIX_SUCCESS != (rc = PMIx_server_finalize())) {
        fprintf(stderr, "Finalize failed with error %d\n", rc);
    }

    return 0;
}
