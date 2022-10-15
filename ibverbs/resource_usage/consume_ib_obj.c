#include "basic_ib.h"
#include "basic_rc.h"

#include <assert.h>
#include <stdio.h>

#define BUF_SIZE 4096
#define CQ_SIZE 1024

#define DEFAULT_NQPS 1
#define DEFAULT_NMRS 1
#define DEFAULT_COMM 0

static void usage(const char *argv0)
{
    printf("Usage:\n");
    printf("  %s allocate specified amount of resources\n", argv0);
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help display this message\n");
    printf("  -d, --dev-name=<dev>   use  device <dev>)\n");
    printf("  -i, --dev_port=<port>  use port <port> of device (default 1)\n");
    printf("  -q, --qps=N Allocate N QPs (default is %d)\n", DEFAULT_NQPS);
    printf("  -m, --mrs=N Allocate N MRs (default is %d)\n", DEFAULT_NQPS);
    printf("  -c, --comm=N Allocate 2xN connected QPs "
           "(overrides '-q' option, default is %d)\n", DEFAULT_COMM);
}

static struct option long_options[] = {
    { .name = "help",       .has_arg = 0, .val = 'h' },
    { .name = "dev-name",  .has_arg = 1, .val = 'd' },
    { .name = "dev-port",  .has_arg = 1, .val = 'i' },
    { .name = "qps",       .has_arg = 1, .val = 'q' },
    { .name = "mrs",       .has_arg = 1, .val = 'p' },
};

static int parse_args(int argc, char **argv, settings_t *s)
{
    memset(s, 0, sizeof(*s));
    char *opts_str = NULL;

    opts_str = "d:i:hq:m:";

    s->dev_port = 1;

    while (1) {
        int c;

        c = getopt_long(argc, argv, opts_str,
                        long_options, NULL);
        if (c == -1)
            break;

        switch (c) {
        case 'd':
            strcpy(s->devname, optarg);
            break;

        case 'i':
            s->dev_port = strtol(optarg, NULL, 0);
            if (s->dev_port < 0) {
                usage(argv[0]);
                return 1;
            }
            break;

        case 'q':
            s->nqps = strtol(optarg, NULL, 0);
            if (s->nqps < 0) {
                usage(argv[0]);
                return 1;
            }
            break;

        case 'm':
            s->nmrs = strtol(optarg, NULL, 0);
            if (s->nmrs < 0) {
                usage(argv[0]);
                return 1;
            }
            break;

        case 'h':
        default:
            usage(argv[0]);
            return 1;
        }
    }
}


int main(int argc, char *argv[])
{
    settings_t settings, *s = &settings;
    ib_context_t ib_ctx, *ctx = &ib_ctx;
    void **mrbufs = NULL;
    ib_mr_t *mrs = NULL;
    ib_cq_t *cqs = NULL;
    ib_qp_t *qps = NULL;
    int i, rc;
    char in_c;

    parse_args(argc, argv, s);

    init_ctx(ctx, s);

    if (s->nmrs) {
        mrbufs = calloc(s->nmrs, sizeof(*mrbufs));
        mrs = calloc(s->nmrs, sizeof(*mrs));
        for (i = 0; i < s->nmrs; i++) {
            mrbufs[i] = calloc(1, BUF_SIZE);
            assert(mrbufs[i]);
            rc = create_mr(&mrs[i], ctx, mrbufs[i], BUF_SIZE);
            assert(!rc);
        }
    }

    if (s->nqps) {
        cqs = calloc(s->nqps, sizeof(*cqs));
        qps = calloc(s->nqps, sizeof(*qps));
        for(i = 0; i < s->nqps; i++) {
            rc = create_cq(&cqs[i], ctx, CQ_SIZE);
            assert(!rc);
            rc = create_qp(&qps[i], &cqs[i]);
            assert(!rc);
        }
    }

    fprintf(stderr, "Resources allocated\nHit any key to quit ...\n");
    fflush(stderr);
    scanf("%c", &in_c);

    if (s->nmrs) {
        for (i = 0; i < s->nmrs; i++) {
            destroy_mr(&mrs[i]);
            free(mrbufs[i]);
        }
        free(mrs);
        free(mrbufs);
    }

    if (s->nqps) {
        for(i = 0; i < s->nmrs; i++) {
            destroy_qp(&qps[i]);
            destroy_cq(&cqs[i]);
        }

        free(qps);
        free(cqs);
    }

    free_ctx(ctx);
    return 0;
}
