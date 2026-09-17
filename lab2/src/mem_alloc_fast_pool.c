#include <assert.h>
#include <stdio.h>

#include "mem_alloc_fast_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

void init_fast_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{
    void *start_adr = my_mmap(size);
    int c = 0;
    mem_fast_free_block_t *block;
    
    if (start_adr != NULL)
    {
        c = size / max_request_size;

        p -> first_free = start_adr;
        p ->total_pool_size = size;

        for (int i = 0; i < c-1; i++)
        {
            block = (mem_fast_free_block_t *)((char *)start_adr + i * max_request_size);
            block ->next = (mem_fast_free_block_t *)((char *)start_adr + (i + 1) * max_request_size);
        } 
        block ->next = NULL;
    
    }
    
}

void *mem_alloc_fast_pool(mem_pool_t *pool, size_t size)
{
    if (pool -> first_free != NULL){

        mem_fast_free_block_t *block = (mem_fast_free_block_t *)pool->first_free;
        void *res = block;
        pool->first_free = block->next;
        
        return res;

    } else {
        printf("No free blocks available in the pool.\n");
        return NULL;
    }
}

void mem_free_fast_pool(mem_pool_t *pool, void *b)
{
    mem_fast_free_block_t *local_var = (mem_fast_free_block_t *)b;
    local_var->next = pool->first_free;
    pool->first_free = local_var;

}

size_t mem_get_allocated_block_size_fast_pool(mem_pool_t *pool, void *addr)
{
    size_t res;
    res = pool->max_req_size;
    return res;
}
