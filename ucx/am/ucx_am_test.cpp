#include <vector>
#include <string>
#include <cassert>
#include <cstring>
#include <iostream>

#include "ucx_utils.hpp"

//TODO: meson conditional build for CUDA
//#define USE_VRAM

using namespace std;

int main()
{
    vector<string> devs;
    devs.push_back("mlx5_0");
    nixl_ucx_worker w[2] = { nixl_ucx_worker(devs), nixl_ucx_worker(devs) };
    nixl_ucx_ep ep[2];
    nixl_ucx_mem mem[2];
    nixl_ucx_rkey rkey[2];
    nixl_ucx_req req;
    uint64_t buffer[2];
    int ret, i;

    uint8_t msg_id = 1;
    size_t msg_len;

    buffer[0] = 0;
    buffer[1] = 0xdeaddeaddeadbeef;

    /* Test control path */
    for(i = 0; i < 2; i++) {
        uint64_t addr;
        size_t size;
        assert(0 == w[i].ep_addr(addr, size));
        assert(0 == w[!i].connect((void*) addr, size, ep[!i]));
	
	//no need for mem_reg with active messages
	//assert(0 == w[i].mem_reg(buffer[i], 128, mem[i]));
        //assert(0 == w[i].mem_addr(mem[i], addr, size));
        //assert(0 == w[!i].rkey_import(ep[!i], (void*) addr, size, rkey[!i]));
        free((void*) addr);
    }

    /* Test active message */

    ret = w[0].reg_am_callback(msg_id);
    assert(ret == 0);

    w[0].progress();

    ret = w[1].send_am(ep[1], msg_id, (void*) &(buffer[1]), sizeof(buffer[1]), req);
    assert(ret == 0);

    while(ret == 0) ret = w[1].test(req);

    std::cout << "active message sent, waiting...\n";

    ret = 0;
    while(ret == 0){
        ret = w[0].get_am_data(&(buffer[0]), msg_len); 
	w[0].progress();
    }

    assert(msg_len == sizeof(uint64_t));
    assert(buffer[0] == 0xdeaddeaddeadbeef);

    /* Test shutdown */
    for(i = 0; i < 2; i++) {
        w[i].rkey_destroy(rkey[i]);
        w[i].mem_dereg(mem[i]);
        assert(0 == w[i].disconnect(ep[i]));
    }

}
