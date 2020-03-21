#include <stdio.h>
#include "utils.h"

char init_symb(bufidx, idx)
{
    char start, end;
    int len;
    switch(bufidx % 3){
    case 0:
        start = 'A';
        end = 'Z';
        break;
    case 1:
        start = 'a';
        end = 'z';
        break;
    case 2:
        start = '0';
        end = '9';
        break;
    }
    return start + idx % (end - start + 1);
}

range_t *init_range(int bufcnt, int repcnt, int *payloads, int *strides)
{
    range_t *r = calloc(1, sizeof(*r));
    int i, j, k, idx;
    
    r->bufcnt = bufcnt;
    r->repcnt = repcnt;
    r->nblocks = repcnt * bufcnt;

    ALLOC(r->payloads, bufcnt);
    memcpy(r->payloads, payloads, bufcnt * sizeof(*r->payloads));
    ALLOC(r->strides, bufcnt);
    memcpy(r->strides, strides, bufcnt * sizeof(*r->strides));
    
    ALLOC(r->insizes, bufcnt);
    ALLOC(r->inbufs, bufcnt);
    for(i = 0; i < bufcnt; i++){
        r->insizes[i] = r->repcnt * r->strides[i];
        r->outsize += r->repcnt * r->payloads[i];
        ALLOC(r->inbufs[i], r->insizes[i]);
        for(j=0; j < r->insizes[i]; j++){
            r->inbufs[i][j] = init_symb(i,j);
        }
    }
    ALLOC(r->outbuf, r->outsize);

    for(j = 0, idx = 0; j < repcnt; j++){
        for(i = 0; i < bufcnt; i++){
            for(k = 0; k < r->payloads[i]; k++){
                r->outbuf[idx++] = r->inbufs[i][ j * r->strides[i] + k];
            }
        }
    }
    return r;
}

void range_fill(range_t *r, char *base_addr, int displs[], int blocklens[], char outbuf[])
{
    int idx, i, j;
    memcpy(outbuf, r->outbuf, r->outsize);
    for(j = 0, idx = 0; j < r->repcnt; j++){
        for(i = 0; i < r->bufcnt; i++){
            blocklens[idx] = r->payloads[i];
            displs[idx] = &r->inbufs[i][ j * r->strides[i]] - base_addr;
            idx++;
        }
    }
}


message_t *message_init(char *base_ptr, int rangeidx, int bufidx, message_desc_t *desc, int desc_cnt)
{
    int i;
    int blk_offs;
    int out_offs;
    message_t *m;

    ALLOC(m, 1);
    m->ranges_cnt = desc_cnt;
    ALLOC(m->ranges, desc_cnt);
    
    for(i=0 ; i < desc_cnt; i++){
        m->ranges[i] = init_range(desc[i].ilv, desc[i].rcnt, desc[i].payloads, desc[i].strides);
        m->nblocks += m->ranges[i]->nblocks;
        m->outlen += m->ranges[i]->outsize;
    }
    ALLOC(m->displs, m->nblocks);
    ALLOC(m->blens, m->nblocks);
    ALLOC(m->outbuf, m->outlen);
    
    if(base_ptr == NULL){
        /* point to the very first buffer */
        base_ptr = m->ranges[rangeidx]->inbufs[bufidx];
    }
    m->base_addr = base_ptr;

    blk_offs = out_offs = 0;
    for(i=0 ; i < desc_cnt; i++){
        range_fill(m->ranges[i], base_ptr, m->displs + blk_offs, m->blens + blk_offs, m->outbuf + out_offs);
        blk_offs += m->ranges[i]->nblocks;
        out_offs += m->ranges[i]->outsize;
    }
    return m;
}