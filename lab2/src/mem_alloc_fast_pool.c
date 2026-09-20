#include <assert.h>
#include <stdio.h>

#include "mem_alloc_fast_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

void init_fast_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{
    /* TO BE IMPLEMENTED */
    //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    
    void *start_adr = my_mmap(size); //size of the memory (in bytes) and starting address of the first byte of this area
    int num_blocks = 0;
    mem_fast_free_block_t *block;

    if (start_adr!=NULL)
    {
        num_blocks = size/max_request_size; // 65536/64 = 1024 blocks of 64 bytes

        p->first_free = start_adr; //the first free block
        p->total_pool_size = size;
        p->min_req_size = min_request_size;
        p->max_req_size = max_request_size;
        p->start_addr = start_adr; //where the pool starts
        p->end_addr = (char *)start_adr + size; //the end address of the pool by moving byte by byte
        p->pool_type = FAST_POOL;

        for (int i=0; i<num_blocks-1; i++)
        {
            //Find the address of the i-th block by moving i blocks from the start
            block = (mem_fast_free_block_t *)((char *)start_adr + i*max_request_size); 

            //It connects each block to the next one.
            block->next = (mem_fast_free_block_t *)((char *)start_adr + (i + 1) * max_request_size);
        }

        //the last block 
        block = (mem_fast_free_block_t *)((char *)start_adr + (num_blocks - 1) * max_request_size);
        //no other free block
        block->next = NULL;
    }
}

void *mem_alloc_fast_pool(mem_pool_t *pool, size_t size)
{
    /* TO BE IMPLEMENTED */
    //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    //return NULL;

    if (pool->first_free != NULL)
    { 
        mem_fast_free_block_t *block =
            (mem_fast_free_block_t *)pool->first_free; //Get the first free block 

        void *allocated_block = block; //Save the address of the allocated block 

        pool->first_free = block->next; //Move first_free to the next free block 
        return allocated_block; //Return the allocated block
    }
    else
    {
        printf("No free blocks available in the pool.\n");
        return NULL;
    }
}

void mem_free_fast_pool(mem_pool_t *pool, void *b)
{
    /* TO BE IMPLEMENTED */
    //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);

    mem_fast_free_block_t *block = (mem_fast_free_block_t *)b; //block being freed
    
    block->next = pool->first_free; //Add the block to the beginning of the free list

    pool->first_free = block; //Make this block the first free  --> LIFO (Last In, First Out)

}

size_t mem_get_allocated_block_size_fast_pool(mem_pool_t *pool, void *addr)
{
    size_t res;
    res = pool->max_req_size;
    return res;
}
