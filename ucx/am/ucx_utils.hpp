#include <ucp/api/ucp.h>

struct nixl_ucx_am_hdr {
    uint64_t whatever;
};

class nixl_ucx_ep {
private:
    //nixl_ucx_worker &uw;
    ucp_ep_h  eph;

public:
    friend class nixl_ucx_worker;
};

class nixl_ucx_mem {
private:
    //nixl_ucx_worker &uw;
    void *base;
    size_t size;
    ucp_mem_h memh;
public:
    friend class nixl_ucx_worker;
};

class nixl_ucx_rkey {
private:
    //nixl_ucx_worker &uw;
    ucp_rkey_h rkeyh;

public:

    friend class nixl_ucx_worker;
};

class nixl_ucx_req {
private:
    //nixl_ucx_worker &uw;
    int complete;
    void* reqh;

public:
    nixl_ucx_req() {
        complete = 0;
    }

    friend class nixl_ucx_worker;
};

class nixl_ucx_worker {
private:
    /* Local UCX stuff */
    ucp_context_h ctx;
    ucp_worker_h worker;

public:

    /* Active msg buffer */
    void* am_buf;
    size_t am_len;

    nixl_ucx_worker(std::vector<std::string> devices);
    //{
        // Create UCX worker spanning provided devices
        // Have a special conf when we want UCX to detect devices
        // automatically
    //}

    ~nixl_ucx_worker(); 

    /* Connection */
    int ep_addr(uint64_t &addr, size_t &size);
    int connect(void* addr, size_t size, nixl_ucx_ep &ep);
    int disconnect(nixl_ucx_ep &ep);

    /* Memory management */
    int mem_reg(void *addr, size_t size, nixl_ucx_mem &mem);
    size_t mem_addr(nixl_ucx_mem &mem, uint64_t &addr, size_t size);
    void mem_dereg(nixl_ucx_mem &mem);

    /* Rkey */
    int rkey_import(nixl_ucx_ep &ep, void* addr, size_t size, nixl_ucx_rkey &rkey);
    void rkey_destroy(nixl_ucx_rkey &rkey);

    /* Active message handling */
    int reg_am_callback(uint16_t msg_id);
    int send_am(nixl_ucx_ep &ep, uint16_t msg_id, void* buffer, size_t len, nixl_ucx_req &req);
    int get_am_data(void* buffer, size_t &len);

    /* Data access */
    int progress();
    int read(nixl_ucx_ep &ep, 
            uint64_t raddr, nixl_ucx_rkey &rk,
            void *laddr, nixl_ucx_mem &mem,
            size_t size, nixl_ucx_req &req);
    int write(nixl_ucx_ep &ep, 
            void *laddr, nixl_ucx_mem &mem,
            uint64_t raddr, nixl_ucx_rkey &rk,
            size_t size, nixl_ucx_req &req);
    int test(nixl_ucx_req &req);

};

