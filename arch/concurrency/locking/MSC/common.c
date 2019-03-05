#include <getopt.h>
#include "common.h"
#include <sys/stat.h>

int nthreads, niter, verify_mode, workload, unlocked_workload, profile;

char base_path[1024];

void set_default_args()
{
    nthreads = DEFAULT_NTHREADS;
    niter = 100;
    verify_mode = 1;
    workload = 1000;
    unlocked_workload = 100;
    profile = 0;
}

void usage(char *cmd)
{
    set_default_args();
    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "    -h  Display this help\n");
    fprintf(stderr, "    -n  Number of threads (default: %d)\n", nthreads);
    fprintf(stderr, "    -i  Number of iterations per thread (default: %d)\n", niter);
    fprintf(stderr, "    -v  Verification mode (default: %d)\n", verify_mode);
    fprintf(stderr, "Non-verification mode only\n");
    fprintf(stderr, "    -p  Profile (default: %s)\n", profile ? "on" : "off");
    fprintf(stderr, "    -w  Workload in the critical section (default: %d)\n", workload);
    fprintf(stderr, "    -u  Workload out of the critical section (default: %d)\n", unlocked_workload);
}

int check_unsigned(char *str)
{
    return (strlen(str) == strspn(str,"0123456789") );
}

void process_args(int argc, char **argv)
{
    int c, rank;
    int verification_flag = 0, workload_flag = 0;

    set_default_args();

    while((c = getopt(argc, argv, "hn:i:v:w:u:p")) != -1) {
        printf("Process option '%c'\n", c);
        switch (c) {
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        case 'p':
    	    printf("Profile: on\n");
    	    profile = 1;
            break;
        case 'n':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            nthreads = atoi(optarg);
            printf("nthreads = %d\n", nthreads);
            break;
        case 'i':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            niter = atoi(optarg);
            printf("niters = %d\n", niter);
            break;
        case 'v':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            verify_mode = !!atoi(optarg);
            printf("verify_mode = %d\n", verify_mode);
            if( verify_mode ){
                verification_flag = 1;
            }
            break;
        case 'w':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            workload= atoi(optarg);
            printf("workload = %d\n", workload);
            workload_flag = 1;
            break;
        case 'u':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            unlocked_workload= atoi(optarg);
            printf("unlocked_workload = %d\n", unlocked_workload);
            break;
        default:
            c = -1;
            goto error;
        }
    }
    
    if( verification_flag && workload_flag ){
	printf("ERROR: workload amount option '-w' only has effect in non-verification mode '-v0'\n");
        usage(argv[0]);
	exit(1);
    }
    
    char *ptr = strrchr(argv[0],'/');
    if( ptr != NULL ) {
	int len = (ptr - argv[0]) + 1;
	strncpy(base_path,argv[0],len);
	base_path[len+1] = '\0';
    }
    
    printf("base path = %s\n", base_path);
    
    return;
error:
    if( c != -1 ){
        fprintf(stderr, "Bad argument of '-%c' option\n", (char)c);
    }
    usage(argv[0]);
    exit(1);
}

void bind_to_core(int thr_idx)
{
    long configured_cpus = 0;
    cpu_set_t set;
    int error_loc = 0, error;
    int ncpus;

    /* How many cpus we have on the node */
    configured_cpus = sysconf(_SC_NPROCESSORS_CONF);

    if( sizeof(set) * 8 < configured_cpus ){
        /* Shouldn't happen soon, currentlu # of cpus = 1024 */
        if( 0 == thr_idx ){
            fprintf(stderr, "The size of CPUSET is larger that we can currently handle\n");
        }
        exit(1);
    }

    if( configured_cpus < thr_idx ){
        fprintf(stderr, "ERROR: (ncpus < thr_idx)\n");
        exit(1);
    }

    CPU_ZERO(&set);
    CPU_SET(thr_idx, &set);
    if( pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
        abort();
    }
}



void get_topo(int ***out_topo, int *nnuma, int *ncores)
{
    char fname[1024];
    char cmd[1024];
    struct stat buf;
    int i;

    *nnuma = -1;
    *ncores = -1;
    *out_topo = NULL;
    sprintf(fname, "%sscripts/%s", base_path, "parse_lstopo.py");
    if( stat(fname, &buf) ){
	printf("WARNING: unable to get topology\n");
	return;
    }
    if( !S_ISREG(buf.st_mode) ){
	printf("WARNING: unable to get topology\n");
	return;
    }
    sprintf(cmd, "python %s", fname);
    FILE *fp = popen(cmd, "r");
    if( NULL == fp ){
	printf("WARNING: unable to get topology\n");
	return;
    }
    fscanf(fp, "%d", nnuma);
    if( 0 > *nnuma ){
	printf("WARNING: unable to get topology\n");
	return;
    }

    fscanf(fp, "%d", ncores);
    if( 0 > *ncores ){
	printf("WARNING: unable to get topology\n");
	return;
    }
    
    *out_topo = calloc(*nnuma, sizeof(**out_topo));
    for(i=0; i < *nnuma; i++) {
	int j;
	(*out_topo)[i] = calloc(*ncores, sizeof(***out_topo));
	for(j = 0; j < *ncores; j++) {
	    fscanf(fp, "%d", &(*out_topo)[i][j]);
	}
    }
    pclose(fp);
}
