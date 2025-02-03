#include <vector>
#include <string>
#include <cstring>
#include <iostream>

#include "ucx_utils.hpp"

using namespace std;

nixl_ucx_worker::nixl_ucx_worker(std::vector<std::string> devs)
{
    ucp_params_t ucp_params;
    ucp_config_t *ucp_config;
    ucp_worker_params_t worker_params;
    ucs_status_t status = UCS_OK;

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES | UCP_PARAM_FIELD_MT_WORKERS_SHARED |
                            UCP_PARAM_FIELD_ESTIMATED_NUM_EPS;
    ucp_params.features = UCP_FEATURE_RMA | UCP_FEATURE_AMO32 | UCP_FEATURE_AMO64;
    ucp_params.mt_workers_shared = 1;
    ucp_params.estimated_num_eps = 3;
    ucp_config_read(NULL, NULL, &ucp_config);

    /* If requested, restrict the set of network devices */
    if (devs.size()) {
        /* TODO: check if this is the best way */
        string dev_str = "";
        unsigned int i;
        for(i=0; i < devs.size() - 1; i++) {
            dev_str = dev_str + devs[i] + ":1,";
        }
        dev_str = dev_str + devs[i] + ":1";
        ucp_config_modify(ucp_config, "NET_DEVICES", dev_str.c_str());
    }

    status = ucp_init(&ucp_params, ucp_config, &ctx);
    if (status != UCS_OK) {
        /* TODO: proper cleanup */
        // TODO: MSW_NET_ERROR(priv->net, "failed to ucp_init(%s)\n", ucs_status_string(status));
        return;
    }
    ucp_config_release(ucp_config);

    memset(&worker_params, 0, sizeof(worker_params));
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    status = ucp_worker_create(ctx, &worker_params, &worker);
    if (status != UCS_OK)
    {
       // TODO: MSW_NET_ERROR(priv->net, "failed to create ucp_worker (%s)\n", ucs_status_string(status));
        return;
    }

    am_buf = NULL;
    am_len = 0;
}

nixl_ucx_worker::~nixl_ucx_worker()
{
    ucp_worker_destroy(worker);
    ucp_cleanup(ctx);
}

int nixl_ucx_worker::ep_addr(uint64_t &addr, size_t &size)
{
    ucp_worker_attr_t wattr;
    ucs_status_t status;
    void* new_addr;

    wattr.field_mask = UCP_WORKER_ATTR_FIELD_ADDRESS |
                       UCP_WORKER_ATTR_FIELD_ADDRESS_FLAGS;
    wattr.address_flags = UCP_WORKER_ADDRESS_FLAG_NET_ONLY;
    status = ucp_worker_query(worker, &wattr);
    if (UCS_OK != status) {
        // TODO: printf
        return -1;
    }

    new_addr = calloc(wattr.address_length, sizeof(char));
    memcpy(new_addr, wattr.address, wattr.address_length);
    ucp_worker_release_address(worker, wattr.address);

    addr = (uint64_t) new_addr;
    size = wattr.address_length;
    return 0;
}

/* ===========================================
 * EP management
 * =========================================== */

static void err_cb(void *arg, ucp_ep_h ep, ucs_status_t status)
{
    printf("error handling callback was invoked with status %d (%s)\n",
           status, ucs_status_string(status));
}


int nixl_ucx_worker::connect(void* addr, size_t size, nixl_ucx_ep &ep)
{
    ucp_ep_params_t ep_params;
    ucs_status_t status;

    //ep.uw = this;

    ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS |
                           UCP_EP_PARAM_FIELD_ERR_HANDLER |
                           UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    ep_params.err_mode = UCP_ERR_HANDLING_MODE_PEER;
    ep_params.err_handler.cb = err_cb;
    ep_params.address = (ucp_address_t*) addr;

    status = ucp_ep_create(worker, &ep_params, &ep.eph);
    if (status != UCS_OK) {
        /* TODO: proper cleanup */
        /* TODO:  MSW_NET_ERROR(priv->net, "!!! failed to create endpoint to remote %d (%s)\n",
                      status, ucs_status_string(status)); */
        return -1;
    }

    return 0;
}

/* TODO: non-blocking disconnect for cleanup performance! */
int nixl_ucx_worker::disconnect(nixl_ucx_ep &ep)
{
    ucs_status_ptr_t request = ucp_ep_close_nb(ep.eph, UCP_EP_CLOSE_MODE_FLUSH);

    if (UCS_PTR_IS_ERR(request)) {
        //TODO: proper cleanup
	//if (UCS_PTR_IS_ERR(request)) {
        //    MSW_NET_ERROR(priv->net, "ucp_disconnect_nb() failed: %s", 
        //                 ucs_status_string(UCS_PTR_STATUS(request)));
        //    return -1;
        //}
	return -1;
    }

    while(ucp_request_check_status(request) == UCS_INPROGRESS) {
        ucp_worker_progress(worker);
    }

    //status is not set anywhere, not sure where it should be set
    //if (status == UCS_INPROGRESS) {
    //    return 0;
    //}

    ucp_request_free(request);

    return 0;
}

/* ===========================================
 * Memory management
 * =========================================== */


int nixl_ucx_worker::mem_reg(void *addr, size_t size, nixl_ucx_mem &mem)
{
    ucs_status_t status;

    //mem.uw = this;
    mem.base = addr;
    mem.size = size;

    ucp_mem_map_params_t mem_params = {
        .field_mask = UCP_MEM_MAP_PARAM_FIELD_FLAGS |
                     UCP_MEM_MAP_PARAM_FIELD_LENGTH |
                     UCP_MEM_MAP_PARAM_FIELD_ADDRESS,
        .address = mem.base,
        .length  = mem.size,
    };

    status = ucp_mem_map(ctx, &mem_params, &mem.memh);
    if (status != UCS_OK) {
        /* TODOL: MSW_NET_ERROR(priv->net, "failed to ucp_mem_map (%s)\n", ucs_status_string(status)); */
        return -1;
    }

    return 0;
}


size_t nixl_ucx_worker::mem_addr(nixl_ucx_mem &mem, uint64_t &addr, size_t size)
{
    ucs_status_t status;
    void *rkey_buf;

    status = ucp_rkey_pack(ctx, mem.memh, &rkey_buf, &size);
    if (status != UCS_OK) {
        /* TODO: MSW_NET_ERROR(priv->net, "failed to ucp_rkey_pack (%s)\n", ucs_status_string(status)); */
        return -1;
    }
    
    /* Allocate the buffer */
    addr = (uint64_t) calloc(size, sizeof(char));
    if (!addr) {
        /* TODO: proper cleanup */
        /* TODO: MSW_NET_ERROR(priv->net, "failed to allocate memory key buffer\n"); */
        return -1;
    }
    memcpy((void*) addr, rkey_buf, size);
    ucp_rkey_buffer_release(rkey_buf);

    return 0;
}

void nixl_ucx_worker::mem_dereg(nixl_ucx_mem &mem)
{
    ucp_mem_unmap(ctx, mem.memh);
}

/* ===========================================
 * RKey management
 * =========================================== */

int nixl_ucx_worker::rkey_import(nixl_ucx_ep &ep, void* addr, size_t size, nixl_ucx_rkey &rkey)
{
    ucs_status_t status;

    status = ucp_ep_rkey_unpack(ep.eph, addr, &rkey.rkeyh);
    if (status != UCS_OK)
    {
        /* TODO: MSW_NET_ERROR(priv->net, "unable to unpack key!\n"); */
        return -1;
    }

    return 0;
}

void nixl_ucx_worker::rkey_destroy(nixl_ucx_rkey &rkey)
{
    ucp_rkey_destroy(rkey.rkeyh);
}

/* ===========================================
 * Active message handling
 * =========================================== */

ucs_status_t ucp_am_nixl_copy_callback(void *arg, const void *header,
			               size_t header_length, void *data,
				       size_t length, 
				       const ucp_am_recv_param_t *param)
{

    struct nixl_ucx_am_hdr* hdr = (struct nixl_ucx_am_hdr*) header;
    nixl_ucx_worker* am_worker = (nixl_ucx_worker*) arg;

    if(hdr->whatever != 0xcee) 
    {
	return UCS_ERR_INVALID_PARAM;
    }

    if(am_worker->am_buf != NULL)
    {
	return UCS_ERR_NO_MEMORY;
    }

    am_worker->am_buf = calloc(1, length);
    am_worker->am_len = length;

    memcpy(am_worker->am_buf, data, length);

    std::cout << "preparing buf " << am_worker->am_buf << "\n";

    return UCS_OK;
}

int nixl_ucx_worker::reg_am_callback(uint16_t msg_id)
{
    ucs_status_t status;
    ucp_am_handler_param_t params;

    params.field_mask = UCP_AM_HANDLER_PARAM_FIELD_ID |
                       UCP_AM_HANDLER_PARAM_FIELD_CB |
                       UCP_AM_HANDLER_PARAM_FIELD_ARG;

    params.id = msg_id;
    params.arg = this;
    params.cb = ucp_am_nixl_copy_callback;

    status = ucp_worker_set_am_recv_handler(worker, &params);

    if(status != UCS_OK) 
    {
        //TODO: error handling
        return -1;
    } 
    return 0;
}

int nixl_ucx_worker::send_am(nixl_ucx_ep &ep, uint16_t msg_id, void* buffer, size_t len, nixl_ucx_req &req)
{
    ucs_status_ptr_t status;
    struct nixl_ucx_am_hdr hdr = {0};
    ucp_request_param_t param = {0};

    hdr.whatever = 0xcee;

    status = ucp_am_send_nbx(ep.eph, msg_id, &hdr, sizeof(hdr), buffer, len, &param);

    if(UCS_PTR_IS_ERR(status)) 
    {
        //TODO: error handling
        return -1;
    }

    req.reqh = status; 
   
    return 0;
}

int nixl_ucx_worker::get_am_data(void* buffer, size_t &len)
{
    if(am_buf == NULL) return 0;

    std::cout << "getting from buf " << am_buf << "\n";
    
    memcpy(buffer, am_buf, am_len);
    len = am_len;

    free(am_buf);
    am_buf = NULL;
    am_len = 0;

    return 1;
}



/* ===========================================
 * Data transfer
 * =========================================== */

int nixl_ucx_worker::progress()
{
    return ucp_worker_progress(worker);
}

int nixl_ucx_worker::read(nixl_ucx_ep &ep, 
            uint64_t raddr, nixl_ucx_rkey &rk,
            void *laddr, nixl_ucx_mem &mem,
            size_t size, nixl_ucx_req &req)
{
    ucs_status_ptr_t request;

    ucp_request_param_t param = {
        .op_attr_mask               = UCP_OP_ATTR_FIELD_MEMH,
        .memh                       = mem.memh,
    };

    request = ucp_get_nbx(ep.eph, laddr, size, raddr, rk.rkeyh, &param);
    if (request == NULL ) {
        goto exit;
    } else if (UCS_PTR_IS_ERR(request)) {
        /* TODO: MSW_NET_ERROR(priv->net, "unable to complete UCX request (%s)\n", 
                         ucs_status_string(UCS_PTR_STATUS(request))); */
        return -1;
    }

exit:
    req.reqh = (void*)request;
    return 0;
}

int nixl_ucx_worker::write(nixl_ucx_ep &ep, 
        void *laddr, nixl_ucx_mem &mem,
        uint64_t raddr, nixl_ucx_rkey &rk,
        size_t size, nixl_ucx_req &req)
{
    ucs_status_ptr_t request;

    ucp_request_param_t param = {
        .op_attr_mask               = UCP_OP_ATTR_FIELD_MEMH,
        .memh                       = mem.memh,
    };

    request = ucp_put_nbx(ep.eph, laddr, size, raddr, rk.rkeyh, &param);
    if (request == NULL ) {
        goto exit;
    } else if (UCS_PTR_IS_ERR(request)) {
        /* TODO: MSW_NET_ERROR(priv->net, "unable to complete UCX request (%s)\n", 
                         ucs_status_string(UCS_PTR_STATUS(request))); */
        return -1;
    }

exit:
    req.reqh = (void*)request;
    return 0;
}

int nixl_ucx_worker::test(nixl_ucx_req &req)
{
    ucs_status_t status;

    //references can't be NULL
    //if (req == NULL) {
    //    return 1;
    //} 

    ucp_worker_progress(worker);
    status = ucp_request_check_status(req.reqh);
    if (status == UCS_INPROGRESS) {
        return 0;
    }

    ucp_request_free(req.reqh);
    return 1;
}
