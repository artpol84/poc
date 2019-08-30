#define BENCHMARK "OSU MPI Multiple Bandwidth / Message Rate Test"
/*
 * Copyright (C) 2002-2019 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University. 
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include <osu_util_mpi.h>

#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif

MPI_Request * mbw_request;
MPI_Status * mbw_reqstat;

double calc_bw(int rank, int size, int num_pairs, int window_size, char *s_buf, char *r_buf);

static int loop_override;
static int skip_override;

uint64_t rdtsc()
{
	unsigned long a, d;
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	return a | ((uint64_t)d << 32);
}

int prof_count = 0;
uint64_t post_cycles = 0, wait_cycles = 0;

int main(int argc, char *argv[])
{
    char *s_buf, *r_buf;
    int numprocs, rank;
    int curr_size;

    loop_override = 0;
    skip_override = 0;

    options.bench = MBW_MR;
    options.subtype = BW;
    
    MPI_CHECK(MPI_Init(&argc, &argv));

    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &numprocs));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    int po_ret = process_options(argc, argv);

    if (PO_OKAY == po_ret && NONE != options.accel) {
        if (init_accel()) {
            fprintf(stderr, "Error initializing device\n");
            exit(EXIT_FAILURE);
        }
    }

    if(options.pairs > (numprocs / 2)) {
        po_ret = PO_BAD_USAGE;
    }

    if (0 == rank) {
        switch (po_ret) {
            case PO_BAD_USAGE:
                print_bad_usage_message(rank);
                break;
            case PO_HELP_MESSAGE:
                usage_mbw_mr();
                break;
            case PO_VERSION_MESSAGE:
                print_version_message(rank);
                MPI_CHECK(MPI_Finalize());
                break;
            case PO_OKAY:
                break;
        }
    }

    switch (po_ret) {
        case PO_BAD_USAGE:
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_FAILURE);
        case PO_HELP_MESSAGE:
        case PO_VERSION_MESSAGE:
            MPI_CHECK(MPI_Finalize());
            exit(EXIT_SUCCESS);
        case PO_OKAY:
            break;
    }

    if (allocate_memory_pt2pt_mul(&s_buf, &r_buf, rank, options.pairs)) {
        /* Error allocating memory */
        MPI_CHECK(MPI_Finalize());
        exit(EXIT_FAILURE);
    }

    if(numprocs < 2) {
        if(rank == 0) {
            fprintf(stderr, "This test requires at least two processes\n");
        }

        MPI_CHECK(MPI_Finalize());

        return EXIT_FAILURE;
    }

    {
        // Print out the hosts to make sure that ranks are distributed as expected
        int i;
        for(i = 0; i < numprocs; i++) {
            MPI_Barrier(MPI_COMM_WORLD);
            if( rank == i ) {
                char hname[1025];
                gethostname(hname, 1024);
                printf("rank %d: %s\n", rank, hname);
            }
        }
        fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if(rank == 0) {
        fprintf(stdout, HEADER);
        print_header(rank, BW);
        fprintf(stdout, "# [ pairs: %d ] [ window size: %d ]\n", options.pairs,
                options.window_size);
        fprintf(stdout, "%-*s%*s%*s\n", 10, "# Size", FIELD_WIDTH,
                "MB/s", FIELD_WIDTH, "Messages/s");
        fflush(stdout);
    }

   /* More than one window size */

    /* Just one window size */
    if (rank < numprocs - 1) {
        mbw_request = (MPI_Request *)malloc(sizeof(MPI_Request) * options.window_size);
        mbw_reqstat = (MPI_Status *)malloc(sizeof(MPI_Status) * options.window_size);
    } else {
        mbw_request = (MPI_Request *)malloc(sizeof(MPI_Request) * options.window_size * (numprocs - 1));
        mbw_reqstat = (MPI_Status *)malloc(sizeof(MPI_Status) * options.window_size * (numprocs - 1));
    }

    for(curr_size = options.min_message_size; curr_size <= options.max_message_size; curr_size *= 2) {
        double bw, rate;

        bw = calc_bw(rank, curr_size, numprocs - 1, options.window_size, s_buf, r_buf);

        if(rank == 0) {
            rate = 1e6 * bw / curr_size;

            if(options.print_rate) {
                fprintf(stdout, "%-*d%*.*f%*.*f\n", 10, curr_size,
                        FIELD_WIDTH, FLOAT_PRECISION, bw, FIELD_WIDTH,
                        FLOAT_PRECISION, rate);
            }

            else {
                fprintf(stdout, "%-*d%*.*f\n", 10, curr_size, FIELD_WIDTH,
                        FLOAT_PRECISION, bw);
            }
        }
    }

   free_memory_pt2pt_mul(s_buf, r_buf, rank, options.pairs);

   MPI_CHECK(MPI_Finalize());

   return EXIT_SUCCESS;
}

double calc_bw(int rank, int size, int writers, int window_size, char *s_buf,
        char *r_buf)
{
    double t_start = 0, t_end = 0, t = 0, sum_time = 0, bw = 0;
    int i, j, target;

	set_buffer_pt2pt(s_buf, rank, options.accel, 'a', size);
	set_buffer_pt2pt(r_buf, rank, options.accel, 'b', size);

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

    if(rank < writers) {
        /* All processes are writing to a single one */
        target = writers;

        for(i = 0; i <  options.iterations +  options.skip; i++) {
            if(i ==  options.skip) {
                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
                t_start = MPI_Wtime();
            }

            for(j = 0; j < window_size; j++) {
                MPI_CHECK(MPI_Isend(s_buf, size, MPI_CHAR, target, 100, MPI_COMM_WORLD,
                        mbw_request + j));
            }
            MPI_CHECK(MPI_Waitall(window_size, mbw_request, mbw_reqstat));
            MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
        }
        t_end = MPI_Wtime();
        t = t_end - t_start;
    } else if(rank == writers) {

        for(i = 0; i <  options.iterations +  options.skip; i++) {
            if(i ==  options.skip) {
                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
                post_cycles = wait_cycles = 0;
            }

            uint64_t start = rdtsc(), end;
            for(j = 0; j < window_size * writers; j++) {
                MPI_CHECK(MPI_Irecv(r_buf, size, MPI_CHAR, MPI_ANY_SOURCE, 100, MPI_COMM_WORLD,
                        mbw_request + j));
            }
            end = rdtsc();
            post_cycles += end - start;
            start = end;

            MPI_CHECK(MPI_Waitall(window_size * writers, mbw_request, mbw_reqstat));
            wait_cycles += rdtsc() - start;

            MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
        }

        double avg_post_cycles = (post_cycles * 1.0) / options.iterations;
        double avg_one_post_cycles = (avg_post_cycles * 1.0) / (window_size * writers);
        double avg_wait_cycles = (1.0 * wait_cycles) / options.iterations;
        printf( "post_cycles = %lu, wait_cycles = %lu\n"
                "avg_post = %lf, avg one post = %lf\n"
                "avg_wait = %lf\n",
                post_cycles, wait_cycles,
                avg_post_cycles, avg_one_post_cycles,
                avg_wait_cycles);

    }

    MPI_CHECK(MPI_Reduce(&t, &sum_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD));

    if(rank == 0) {
        double tmp = size / 1e6 * writers ;

        sum_time /= writers;
        tmp = tmp *  options.iterations * window_size;
        bw = tmp / sum_time;

        return bw;
    }

    return 0;
}

/* vi: set sw=4 sts=4 tw=80: */
