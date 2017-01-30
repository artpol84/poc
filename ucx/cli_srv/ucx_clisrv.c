#include <stdio.h>
#include <poll.h>
#include <ucp/api/ucp.h>
#include <ucp/api/ucp_def.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

extern int errno;
#include <errno.h>

int rank, size;
int nmessages = 5;

void basta()
{
	MPI_Abort(MPI_COMM_WORLD, 0);
	exit(0);
}

/* UCP handler objects */
ucp_context_h ucp_context;
ucp_worker_h ucp_worker;

static ucp_address_t *server_addr;
static size_t server_addr_len;

struct ucx_context {
    int completed;
};

struct test_message {
    int rank;
    int str[5];
};

struct ucx_context request;

static void request_init(void *request)
{
    struct ucx_context *ctx = (struct ucx_context *) request;
    ctx->completed = 0;
}

static void send_handle(void *request, ucs_status_t status)
{
    struct ucx_context *context = (struct ucx_context *) request;
    context->completed = 1;
}

static void recv_handle(void *request, ucs_status_t status,
                       ucp_tag_recv_info_t *info)
{
    struct ucx_context *context = (struct ucx_context *) request;
    context->completed = 1;
}

int prepare_ucx()
{
    ucp_config_t *config;
    ucs_status_t status;
    ucp_params_t ucp_params;
    ucp_worker_params_t worker_params;
    unsigned long len = server_addr_len;
    
    status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK) {
        basta();
    }

    ucp_params.features = UCP_FEATURE_TAG | UCP_FEATURE_WAKEUP;
    ucp_params.request_size    = sizeof(struct ucx_context);
    ucp_params.request_init    = request_init;
    ucp_params.request_cleanup = NULL;
    ucp_params.field_mask      = UCP_PARAM_FIELD_FEATURES |
                             UCP_PARAM_FIELD_REQUEST_SIZE |
                             UCP_PARAM_FIELD_REQUEST_INIT |
                             UCP_PARAM_FIELD_REQUEST_CLEANUP;
    
    status = ucp_init(&ucp_params, config, &ucp_context);
//    ucp_config_print(config, stdout, NULL, UCS_CONFIG_PRINT_CONFIG);
    ucp_config_release(config);
    if (status != UCS_OK) {
        basta();
    }
    
    worker_params.field_mask  = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    status = ucp_worker_create(ucp_context, &worker_params, &ucp_worker);
    if (status != UCS_OK) {
        basta();
    }
    
    if( 0 == rank ){
        status = ucp_worker_get_address(ucp_worker, &server_addr, &server_addr_len);
        if (status != UCS_OK) {
            basta();
        }
        len = server_addr_len;
        MPI_Bcast(&len, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        MPI_Bcast(server_addr, server_addr_len, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        MPI_Bcast(&len, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        server_addr_len = len;
        server_addr = malloc( server_addr_len );
        MPI_Bcast(server_addr, server_addr_len, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
}

void cleanup_ucx()
{
    if( 0 == rank ){
        ucp_worker_release_address(ucp_worker, server_addr);
    }
    ucp_worker_destroy(ucp_worker);
    ucp_cleanup(ucp_context);
}

void server_operation()
{
    int ranks_account[size - 1];
    ucs_status_t status;
    int fd, flag = 1;
    struct test_message msg;
    struct ucx_context *request = 0;
    ucp_tag_message_h msg_tag;
    ucp_tag_recv_info_t info_tag;
    int i, cur;

    memset(ranks_account, 0, sizeof(ranks_account));

    /* get fd to poll on */
    status = ucp_worker_get_efd(ucp_worker, &fd);
    if (status != UCS_OK) {
        basta();
    }

    do {
        struct pollfd pfd = { fd, POLLIN, 0 };
        int rc;

        status = ucp_worker_arm(ucp_worker);
        if (status == UCS_ERR_BUSY) { /* some events are arrived already */
            goto process;
        }
        if( status != UCS_OK ){
            basta();
        }

        rc = poll(&pfd, 1, -1);

        if( rc < 0 ){
            if( errno == EINTR ){
                continue;
            } else {
                basta();
            }
        }
        if( !(pfd.revents & POLLIN) ){
            continue;
        }
process:
        do { 
            ucp_worker_progress(ucp_worker);
            msg_tag = ucp_tag_probe_nb(ucp_worker,1, 0xffffffffffffffff, 1, &info_tag);
        
            if( NULL == msg_tag ){
                break;
            }
            if( info_tag.length != sizeof(struct test_message) ){
                printf("Bad message!\n");
                basta();
            }
            request = (struct ucx_context*)
                    ucp_tag_msg_recv_nb(ucp_worker, (void*)&msg, info_tag.length,
                                        ucp_dt_make_contig(1), msg_tag, recv_handle);
            while( request->completed == 0 ){
                ucp_worker_progress(ucp_worker);
            }
            ucp_request_release(request);
        
            //printf("Message from %d\n", msg.rank);
            cur = ranks_account[msg.rank - 1]++;
            for(i=0; i < 5; i++){
                if( msg.str[i] != i + cur * 5 ){
                    printf("%d: Mismatch from rank %d\n", rank, msg.rank);
                }
            }
        } while( 1 );
        /* check the completion */
        flag = 0;
        for(i=0; i<size-1; i++){
            if( ranks_account[i] != 5 ){
                flag = 1;
                break;
            }
        }
    } while( flag );
}

void client_operation()
{
    ucs_status_t status;
    struct test_message msg;
    ucp_ep_params_t ep_params;
    ucp_ep_h server_ep;
    struct ucx_context *request = 0;
    int i;

    /* Send client UCX address to server */
    ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
    ep_params.address    = server_addr;
    
    status = ucp_ep_create(ucp_worker, &ep_params, &server_ep);
    if (status != UCS_OK) {
        basta();
    }
    
    msg.rank = rank;
    for(i = 0; i < 5; i++){
        int j;
        for(j=0; j< 5; j++){
            msg.str[j] = i * 5 + j;
        }
        request = ucp_tag_send_nb(server_ep, (void*)&msg, sizeof(msg),
                                    ucp_dt_make_contig(1), 1, send_handle);
        if (UCS_PTR_IS_ERR(request)) {
            printf("%d: unable to send UCX address message\n", rank);
            basta();
        } else if (UCS_PTR_STATUS(request) != UCS_OK) {
            while( request->completed == 0 ){
                 ucp_worker_progress(ucp_worker);
            }
            ucp_request_release(request);
        }
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if( 0 == rank ){
        int delay = 0;
        while( delay ){
            sleep(1);
        }
    }
    
    prepare_ucx();
    
    if( 0 == rank ){
        server_operation();
        printf("SUCCESS\n");
    } else {
        client_operation();
    }
    
    cleanup_ucx();
    MPI_Finalize();
}
