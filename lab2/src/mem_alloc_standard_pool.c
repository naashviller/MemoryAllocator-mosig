#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "mem_alloc_types.h"
#include "mem_alloc_standard_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

/////////////////////////////////////////////////////////////////////////////

#ifdef STDPOOL_POLICY
/* Get the value provided by the Makefile */
std_pool_placement_policy_t std_pool_policy = STDPOOL_POLICY;
#else
std_pool_placement_policy_t std_pool_policy = DEFAULT_STDPOOL_POLICY;
#endif

/////////////////////////////////////////////////////////////////////////////

void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{
    void *start_adr = my_mmap(size); //size of the memory (in bytes) and starting address of the first byte of this area
    if (start_adr!=NULL)
    {
        mem_std_free_block_t *block = (mem_std_free_block_t *)start_adr; //the first free block
        size_t payload_size = size - sizeof(mem_std_block_header_footer_t) - sizeof(mem_std_block_header_footer_t); //the payload size of the block
        
        p->total_pool_size = size;
        p->min_req_size = min_request_size;
        p->max_req_size = max_request_size;
        p->start_addr = start_adr; //where the pool starts
        p->end_addr = (char *)start_adr + size; //the end address of the pool by moving byte by byte
        p->first_free = block; //the first free block
        p->pool_type = STANDARD_POOL;

        //the pool contains one free block first
        block->prev = NULL; //no previous block
        block->next = NULL; //no next block

        //the header 
        set_block_free(&(block->header)); //the block is free
        set_block_size(&(block->header), payload_size); //the size of the block

        //the footer
        mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)start_adr + size - sizeof(mem_std_block_header_footer_t)); 

        set_block_free(footer); //the block is free
        set_block_size(footer, payload_size); //the size of the block
    }
}

void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size)
{
    mem_std_free_block_t *block = (mem_std_free_block_t *)pool->first_free; //the first free block
    while (block != NULL)
    {
        if (get_block_size(&(block->header))>=size)
        {
            break;
        }
        block = block->next; 
    }

    if (block == NULL)
    {
        return NULL;
    }

    size_t old_payload_size = get_block_size(&block->header);
    size_t allocated_block_size = size + sizeof(mem_std_block_header_footer_t) + sizeof(mem_std_block_header_footer_t); //the size of the allocated block
    size_t min_free_block_size = sizeof(mem_std_free_block_t) + sizeof(mem_std_block_header_footer_t) + sizeof(mem_std_block_header_footer_t); //the minimum size of a free block
    size_t old_block_size = old_payload_size + sizeof(mem_std_block_header_footer_t) + sizeof(mem_std_block_header_footer_t); //the size of the old block
    size_t remaining_size = old_block_size - allocated_block_size; //the remaining size of the block

    if (remaining_size >= min_free_block_size)
    {
        mem_std_free_block_t *new_free_block = (mem_std_free_block_t *)((char *)block + allocated_block_size); //the new free block
        new_free_block->prev = block->prev; //the previous block of the new free block is the previous block of the old block
        new_free_block->next = block->next; //the next block of the new free block is the next block of the old block               
    
        if (new_free_block->prev !=NULL)
        {
            new_free_block->prev->next = new_free_block; //the next block of the previous block is the new free block
        }
        else
        {
            pool->first_free = new_free_block; //the first free block is the new free block
        }

        if (new_free_block->next !=NULL)
        {
            new_free_block->next->prev = new_free_block; //the previous block of the next block is the new free block
        }

        //Initialize the new free block
        size_t new_free_payload_size = remaining_size - sizeof(mem_std_block_header_footer_t) - sizeof(mem_std_block_header_footer_t); //the payload size of the new free block
        
        //the header of the new free block
        set_block_free(&(new_free_block->header)); //the new free block is free
        set_block_size(&(new_free_block->header), new_free_payload_size); //the size
        
        //the footer of the new free block
        mem_std_block_header_footer_t *new_free_footer = (mem_std_block_header_footer_t *)((char *)new_free_block + sizeof(mem_std_block_header_footer_t) + new_free_payload_size); //
        set_block_free(new_free_footer); //the new free block is free
        set_block_size(new_free_footer, new_free_payload_size); //the size
        
        //allocated old block
        set_block_used(&(block->header));
        set_block_size(&(block->header), size);

        //the footer of the allocated block
        mem_std_block_header_footer_t *allocated_footer = (mem_std_block_header_footer_t *)((char *)block + sizeof(mem_std_block_header_footer_t) + size);
        set_block_used(allocated_footer);
        set_block_size(allocated_footer, size);
    
        }
        else
        {
            if (block->prev != NULL)
            {
                block->prev->next = block->next; //the next block of the previous block is the next block of the old block
            }
            else
            {
                pool->first_free = block->next; //the first free block is the next block of the old block
            }

            if (block->next != NULL)
            {
                block->next->prev = block->prev; //the previous block of the next block is the previous block of the old block
            }

            set_block_used(&(block->header)); //the block is used
            set_block_size(&(block->header), old_payload_size); //the size of the block

            mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)block + sizeof(mem_std_block_header_footer_t) + old_payload_size); //the footer of the block
            set_block_used(footer); //the block is used
            set_block_size(footer, old_payload_size); //the size of the block
        }

    return (void *)((char *)block + sizeof(mem_std_block_header_footer_t)); //return the address of the payload of the block
}

void mem_free_standard_pool(mem_pool_t *pool, void *addr)
{    
    mem_std_block_header_footer_t *header = (mem_std_block_header_footer_t *)((char *)addr - sizeof(mem_std_block_header_footer_t)); 

    size_t block_size = get_block_size(header);

    mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)addr + block_size);
    
    // Mark the header and footer as free
    set_block_free(header);
    set_block_free(footer);

    mem_std_free_block_t *current_block = (mem_std_free_block_t *)pool->first_free;
    mem_std_free_block_t *previous_block = NULL;

    while(current_block != NULL && (char *)current_block < (char *)header)
    {
        previous_block = current_block;
        current_block = current_block->next;
    }

    mem_std_free_block_t *new_free_block = (mem_std_free_block_t *)header;
    new_free_block->prev = previous_block;
    new_free_block->next = current_block;

    if (previous_block != NULL)
    {
        previous_block->next = new_free_block;
    }
    else
    {
        pool->first_free = new_free_block;
    }

    if (current_block != NULL)
    {
        current_block->prev = new_free_block;
    }


    //Coalescing with previous block
    mem_std_block_header_footer_t *previous_footer = (mem_std_block_header_footer_t *)((char *)new_free_block - sizeof(mem_std_block_header_footer_t));

    if (previous_block !=NULL && is_block_free(previous_footer))
    {
        //Τhe size of the merged block
        size_t new_size = get_block_size(&previous_block->header) + sizeof(mem_std_block_header_footer_t) + sizeof(mem_std_block_header_footer_t) + get_block_size(&new_free_block->header);
        
        //New header for the merged block
        set_block_free(&previous_block->header);
        set_block_size(&previous_block->header, new_size);

        //Νew footer at the end of the merged block
        mem_std_block_header_footer_t *new_footer = (mem_std_block_header_footer_t *)((char *)previous_block + sizeof(mem_std_block_header_footer_t) + new_size);
        
        //New footer for the merged block
        set_block_free(new_footer);
        set_block_size(new_footer, new_size);   

        previous_block->next = current_block;
        if (current_block != NULL)
        {
            current_block->prev = previous_block;
        }

        new_free_block = previous_block;
    }

    //Coalescing with next block
    if (current_block != NULL) 
    { 
        mem_std_block_header_footer_t *new_free_footer = (mem_std_block_header_footer_t *) ((char *)new_free_block 
                                        + sizeof(mem_std_block_header_footer_t) + get_block_size(&new_free_block->header)); 
        
        if ((char *)new_free_footer + sizeof(mem_std_block_header_footer_t) == (char *)current_block) 
        { 
            size_t new_size = get_block_size(&new_free_block->header) + sizeof(mem_std_block_header_footer_t) + sizeof(mem_std_block_header_footer_t) 
            + get_block_size(&current_block->header); mem_std_free_block_t *next_block = current_block->next; 

           set_block_free(&new_free_block->header); 
           set_block_size(&new_free_block->header, new_size); 

           mem_std_block_header_footer_t *new_footer = (mem_std_block_header_footer_t *) ((char *)current_block + sizeof(mem_std_block_header_footer_t) 
           + get_block_size(&current_block->header)); 
           set_block_free(new_footer); 
           set_block_size(new_footer, new_size); 
           
           new_free_block->next = next_block; 
           if (next_block != NULL) 
           { 
            next_block->prev = new_free_block; 
            } 
        } 
    }
}

// Find the block header and return the size of the allocated block
size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    mem_std_block_header_footer_t *header =
        (mem_std_block_header_footer_t *)((char *)addr - sizeof(mem_std_block_header_footer_t));

    return get_block_size(header);
}
