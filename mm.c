/*
 * =====================================================================================
 *
 *       Filename:  mm.c
 *
 *    Description:  This file implements the routine for Memory Manager 
 *
 *        Version:  1.0
 *        Created:  01/30/2020 10:31:41 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.wixsite.com/csepracticals), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the Linux Memory Manager distribution (https://github.com/sachinites) 
 *        Copyright (c) 2019 Abhishek Sagar.
 *        This program is free software: you can redistribute it and/or modify it under the terms of the GNU General 
 *        Public License as published by the Free Software Foundation, version 3.
 *        
 *        This program is distributed in the hope that it will be useful, but
 *        WITHOUT ANY WARRANTY; without even the implied warranty of
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *        General Public License for more details.
 *
 *        visit website : https://csepracticals.wixsite.com/csepracticals for more courses and projects
 *                                  
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h> /*for getpagesize, brk(), sbrk()*/
#include <errno.h>
#include "css.h"
#include "mm.h"

/*Library Globals*/
void          *gb_heap_segment_start = NULL;
size_t         GB_SYSTEM_PAGE_SIZE = 0;
uint32_t       gb_no_of_vm_families_registered = 0;
void          *gb_hsba = NULL; /*Heap Segment Start for Block Allocation*/

void
mm_init(){

    GB_SYSTEM_PAGE_SIZE = getpagesize();
    gb_heap_segment_start = sbrk(0);
    if(!gb_heap_segment_start){
        printf("Heap Memory Instantiation Failed\n");
        assert(0);
    }
}
#if 1
vm_page_t *
mm_get_available_page_from_heap_segment(){

    vm_page_t *vm_page_curr = NULL;
#if 1
    vm_page_t *first_vm_page = (vm_page_t *)gb_hsba;

    ITERATE_HEAP_SEGMENT_PAGE_WISE_BEGIN(first_vm_page, vm_page_curr){
        if(mm_is_vm_page_empty(vm_page_curr)){
            return vm_page_curr;
        }
    }ITERATE_HEAP_SEGMENT_PAGE_WISE_END(first_vm_page, vm_page_curr);
#endif
    /*No free Page could be found, expand heap segment*/
    
    vm_page_curr = (vm_page_t *)sbrk(GB_SYSTEM_PAGE_SIZE);

    if(!vm_page_curr){
        printf("Error : Heap Segment Expansion Failed, error no = %d\n", errno);
        return 0;
    }
#if 0
    printf("Heap Segment Expanded. New diff = %lu, page units from gb_hsba = %ld, new sbrk = %p\n",
        (unsigned long)sbrk(0) - (unsigned long)initial_break,
        ((unsigned long)sbrk(0) - (unsigned long)gb_hsba)/GB_SYSTEM_PAGE_SIZE,
        sbrk(0));
#endif
    return vm_page_curr;
}
#endif

#if 0
vm_page_t *
mm_get_available_page_from_heap_segment(){

    vm_page_t *vm_page_curr = NULL;
    /*No free Page could be found, expand heap segment*/
    vm_page_curr = (vm_page_t *)sbrk(GB_SYSTEM_PAGE_SIZE);
    return vm_page_curr;
}
#endif

static inline uint32_t
mm_max_page_allocatable_memory(){

    return (uint32_t)
        (GB_SYSTEM_PAGE_SIZE - offset_of(vm_page_t, page_memory));
}

#define MAX_PAGE_ALLOCATABLE_MEMORY \
    (mm_max_page_allocatable_memory())

static vm_page_t *
mm_get_available_page_index(vm_page_family_t *vm_page_family){

    vm_page_t *curr, *prev;
    int page_index = -1;

    if(!vm_page_family->first_page)
        return NULL;

    ITERATE_VM_PAGE_PER_FAMILY_BEGIN(vm_page_family, curr){

        if((int)(curr->page_index) == page_index + 1){
            page_index++;
            prev = curr;
            continue;
        }
        return curr->prev;
    } ITERATE_VM_PAGE_PER_FAMILY_END(vm_page_family, curr)

    return prev;
}

/*Return a fresh new virtual page*/
vm_page_t *
allocate_vm_page(vm_page_family_t *vm_page_family){

    vm_page_t *prev_page = 
        mm_get_available_page_index(vm_page_family);

    vm_page_t *vm_page = 
        mm_get_available_page_from_heap_segment();

    vm_page->block_meta_data.is_free = MM_TRUE;
    vm_page->block_meta_data.block_size = 
        MAX_PAGE_ALLOCATABLE_MEMORY;
    vm_page->block_meta_data.offset = 
        offset_of(vm_page_t, block_meta_data);
    init_glthread(&vm_page->block_meta_data.priority_thread_glue);
    vm_page->block_meta_data.prev_block = NULL;
     vm_page->block_meta_data.next_block = NULL;
    vm_page->next = NULL;
    vm_page->prev = NULL;
    vm_page_family->no_of_system_calls_to_alloc_dealloc_vm_pages++;
    vm_page->pg_family = vm_page_family;

    if(!prev_page){
        vm_page->page_index = 0;
        vm_page->next = vm_page_family->first_page;
        if(vm_page_family->first_page)
            vm_page_family->first_page->prev = vm_page;
        vm_page_family->first_page = vm_page;
        return vm_page;
    }

    vm_page->next = prev_page->next;
    vm_page->prev = prev_page;
    if(vm_page->next)
        vm_page->next->prev = vm_page;
    prev_page->next = vm_page;
    vm_page->page_index = prev_page->page_index + 1;
    return vm_page;
}
void
mm_instantiate_new_page_family(
    char *struct_name,
    uint32_t struct_size){

    vm_page_family_t *vm_page_family = NULL;
    static size_t remaining_bytes_in_a_page = 0;

    if(struct_size > GB_SYSTEM_PAGE_SIZE){
        printf("Error : %s() Structure Size exceeds system page size\n",
            __FUNCTION__);
        return;
    }

    if(!gb_no_of_vm_families_registered){
        
        vm_page_family = (vm_page_family_t *)sbrk(GB_SYSTEM_PAGE_SIZE);
        remaining_bytes_in_a_page = GB_SYSTEM_PAGE_SIZE - sizeof(vm_page_family_t);
        gb_hsba = (void *)
            ((char *)vm_page_family + GB_SYSTEM_PAGE_SIZE);
    }
    else if(remaining_bytes_in_a_page >= sizeof(vm_page_family_t)){
        
        vm_page_family = (vm_page_family_t *)((char *)gb_heap_segment_start + \
            GB_SYSTEM_PAGE_SIZE - remaining_bytes_in_a_page);

        remaining_bytes_in_a_page -= sizeof(vm_page_family_t);
    }
    else{
        vm_page_family = (vm_page_family_t *)sbrk(GB_SYSTEM_PAGE_SIZE);
        remaining_bytes_in_a_page = GB_SYSTEM_PAGE_SIZE - sizeof(vm_page_family_t);
        gb_hsba = (void *)
            ((char *)vm_page_family + GB_SYSTEM_PAGE_SIZE);
    }
    gb_no_of_vm_families_registered++;
    strncpy(vm_page_family->struct_name, struct_name, MM_MAX_STRUCT_NAME);
    vm_page_family->struct_size = struct_size;
    vm_page_family->first_page = NULL;
    init_glthread(&vm_page_family->free_block_priority_list_head);
}

vm_page_family_t *
lookup_page_family_by_name(char *struct_name){

    vm_page_family_t *vm_page_family_curr;

    ITERATE_PAGE_FAMILIES_BEGIN(gb_heap_segment_start, 
        vm_page_family_curr){

        if(strncmp(vm_page_family_curr->struct_name,
            struct_name,
            MM_MAX_STRUCT_NAME) == 0){

            return vm_page_family_curr;
        }
    }ITERATE_PAGE_FAMILIES_END(gb_heap_segment_start, vm_page_family_curr);
    return NULL;
}

static int
free_blocks_comparison_function(
        void *_block_meta_data1,
        void *_block_meta_data2){

    block_meta_data_t *block_meta_data1 = 
        (block_meta_data_t *)_block_meta_data1;

    block_meta_data_t *block_meta_data2 = 
        (block_meta_data_t *)_block_meta_data2;

    if(block_meta_data1->block_size > block_meta_data2->block_size)
        return -1;
    else if(block_meta_data1->block_size < block_meta_data2->block_size)
        return 1;
    return 0;
}

static void
mm_add_free_block_meta_data_to_free_block_list(
        vm_page_family_t *vm_page_family, 
        block_meta_data_t *free_block){

    assert(free_block->is_free == MM_TRUE);
    glthread_priority_insert(&vm_page_family->free_block_priority_list_head, 
            &free_block->priority_thread_glue,
            free_blocks_comparison_function,
            offset_of(block_meta_data_t, priority_thread_glue));
}

static vm_page_t *
mm_family_new_page_add(vm_page_family_t *vm_page_family){

    vm_page_t *vm_page = allocate_vm_page(vm_page_family);

    if(!vm_page)
        return NULL;

    /* The new page is like one free block, add it to the
     * free block list*/
    mm_add_free_block_meta_data_to_free_block_list(
        vm_page_family, &vm_page->block_meta_data);

    return vm_page;
}

/* Fn to mark block_meta_data as being Allocated for
 * 'size' bytes of application data. Return TRUE if 
 * block allocation succeeds*/
static vm_bool_t
mm_allocate_free_block(
            vm_page_family_t *vm_page_family,
            block_meta_data_t *block_meta_data, 
            uint32_t size){

    assert(block_meta_data->is_free == MM_TRUE);

    assert(block_meta_data->block_size >= size);

    uint32_t remaining_size = 
            block_meta_data->block_size - size;

    block_meta_data->is_free = MM_FALSE;
    block_meta_data->block_size = size;
 
    /*Unchanged*/
    //block_meta_data->offset =  ??
    
    /* Since this block of memory is not allocated, remove it from
     * priority list of free blocks*/
    remove_glthread(&block_meta_data->priority_thread_glue);

    vm_page_family->total_memory_in_use_by_app += 
        sizeof(block_meta_data_t) + size;
    /* No need to do anything else if this block is completely used
     * to satisfy memory request*/
    if(!remaining_size)
        return MM_TRUE;

    /* If the remaining memory chunk in this free block is unusable
     * because of fragmentation - however this should not be possible
     * except the boundry condition*/
    if(remaining_size < 
        (sizeof(block_meta_data_t) + vm_page_family->struct_size)){
        /*printf("Warning : %uB Memory Unusable at page bottom\n", 
            remaining_size);*/
        return MM_TRUE;
    }

    block_meta_data_t *next_block_meta_data = NULL;

    next_block_meta_data = NEXT_META_BLOCK_BY_SIZE(block_meta_data);

    next_block_meta_data->is_free = MM_TRUE;

    next_block_meta_data->block_size = 
        remaining_size - sizeof(block_meta_data_t);

    next_block_meta_data->offset = block_meta_data->offset + 
        sizeof(block_meta_data_t) + block_meta_data->block_size;

    init_glthread(&next_block_meta_data->priority_thread_glue); 
    
    mm_bind_blocks_for_allocation(block_meta_data, next_block_meta_data);
    
    mm_add_free_block_meta_data_to_free_block_list(
            vm_page_family, next_block_meta_data);

    return MM_TRUE;
}

static vm_page_t *
mm_get_page_satisfying_request(
        vm_page_family_t *vm_page_family,
        uint32_t req_size, 
        block_meta_data_t **block_meta_data/*O/P*/){

    vm_bool_t status = MM_FALSE;
    vm_page_t *vm_page = NULL;

    block_meta_data_t *biggest_block_meta_data = 
        mm_get_biggest_free_block_page_family(vm_page_family); 

    if(!biggest_block_meta_data || 
        biggest_block_meta_data->block_size < req_size){

        /*Time to add a new page to Page family to satisfy the request*/
        vm_page = mm_family_new_page_add(vm_page_family);
        /*Allocate the free block from this page now*/
        status = mm_allocate_free_block(vm_page_family, 
                    &vm_page->block_meta_data, req_size);

        if(status == MM_FALSE){
            *block_meta_data = NULL;
             mm_vm_page_delete_and_free(vm_page);
             return NULL;
        }

        *block_meta_data = &vm_page->block_meta_data;
        return vm_page;
    }
    /*The biggest block meta data can satisfy the request*/
    status = mm_allocate_free_block(vm_page_family, 
        biggest_block_meta_data, req_size);
        
    if(status == MM_FALSE){
        *block_meta_data = NULL;
        return NULL;
    }

    *block_meta_data = biggest_block_meta_data;

    return MM_GET_PAGE_FROM_META_BLOCK(biggest_block_meta_data);
}

/* The public fn to be invoked by the application for Dynamic 
 * Memory Allocations.*/
void *
xcalloc(char *struct_name, int units){

    void *result = NULL;

    vm_page_family_t *pg_family = 
        lookup_page_family_by_name(struct_name);

    if(!pg_family){
        
        printf("Error : Structure %s not registered with Memory Manager\n",
            struct_name);
        return NULL;
    }
    
    if(units * pg_family->struct_size > MAX_PAGE_ALLOCATABLE_MEMORY){
        
        printf("Error : Memory Requested Exceeds Page Size\n");
        return NULL;
    }

    if(!pg_family->first_page){

        pg_family->first_page = mm_family_new_page_add(pg_family);

        if(mm_allocate_free_block(pg_family, 
                    &pg_family->first_page->block_meta_data, 
                    units * pg_family->struct_size)){
            memset((char *)pg_family->first_page->page_memory, 0,
                units * pg_family->struct_size);
            return (void *)pg_family->first_page->page_memory;
        }
    }
    
    /*Find the page which can satisfy the request*/
    block_meta_data_t *free_block_meta_data;

    vm_page_t *vm_page_curr = mm_get_page_satisfying_request(
                        pg_family, units * pg_family->struct_size,
                        &free_block_meta_data);
    
    if(free_block_meta_data){
        /*Sanity Checks*/
        if(free_block_meta_data->is_free == MM_TRUE ||
                !IS_GLTHREAD_LIST_EMPTY(&free_block_meta_data->priority_thread_glue)){
            assert(0);
        }
        memset((char *)(free_block_meta_data + 1), 0, 
            free_block_meta_data->block_size);
        return  (void *)(free_block_meta_data + 1);
    }

    return NULL;
}

static void
mm_union_free_blocks(block_meta_data_t *first,
        block_meta_data_t *second){

    assert(first->is_free == MM_TRUE &&
        second->is_free == MM_TRUE);

    first->block_size += sizeof(block_meta_data_t) +
            second->block_size;
    remove_glthread(&first->priority_thread_glue);
    remove_glthread(&second->priority_thread_glue);
    mm_bind_blocks_for_deallocation(first, second);
}

static void
mm_return_vm_page_to_heap_segment(vm_page_t *vm_page){

    MARK_VM_PAGE_EMPTY(vm_page);

    /* If this VM page is the top-most page of Heap Memory
     * Segment, then lower down the heap memory segment.
     * Note that, once you lower down the heap memory segment
     * this page shall be out of allotted valid virtual address 
     * of a process, and any access to it shall result in
     * segmentation fault*/
    /* Also note that, if the VM page is the top-most page of Heap Memory
     * then it could be possible there are free contiguous pages below
     * this VM page. We need to lowered down break pointer freeing all
     * contiguous VM pages lying below this VM page*/
    if((void *)vm_page != 
            (void *)((char *)sbrk(0) - GB_SYSTEM_PAGE_SIZE)){
        return;
    }

    vm_page_t *bottom_most_free_page = NULL;

    for(bottom_most_free_page = 
            MM_GET_NEXT_PAGE_IN_HEAP_SEGMENT(vm_page, '-');
        mm_is_vm_page_empty(bottom_most_free_page);
        bottom_most_free_page = 
            MM_GET_NEXT_PAGE_IN_HEAP_SEGMENT(bottom_most_free_page, '-')){
        
        if((void *)bottom_most_free_page == gb_hsba)
            break;
    }

    if((void *)bottom_most_free_page != gb_hsba){
        bottom_most_free_page = 
            MM_GET_NEXT_PAGE_IN_HEAP_SEGMENT(bottom_most_free_page, '+');
    }
#if 0
    printf("No of Contiguous pages to be freed from Heap Segment = %lu\n",
        (((char *)vm_page - (char *)bottom_most_free_page)/GB_SYSTEM_PAGE_SIZE)+ 1);
#endif
    /*Now lower down the break pointer*/
    assert(!brk((void *)bottom_most_free_page));
}

void
mm_vm_page_delete_and_free(
        vm_page_t *vm_page){

    vm_page_family_t *vm_page_family = 
        vm_page->pg_family;

    assert(vm_page_family->first_page);

    if(vm_page_family->first_page == vm_page){
        vm_page_family->first_page = vm_page->next;
        if(vm_page->next)
            vm_page->next->prev = NULL;
        vm_page_family->no_of_system_calls_to_alloc_dealloc_vm_pages++;
        mm_return_vm_page_to_heap_segment(vm_page);
        return;
    }

    if(vm_page->next)
        vm_page->next->prev = vm_page->prev;
    vm_page->prev->next = vm_page->next;
    vm_page_family->no_of_system_calls_to_alloc_dealloc_vm_pages++;
    mm_return_vm_page_to_heap_segment(vm_page);
}

static block_meta_data_t *
mm_free_blocks(block_meta_data_t *to_be_free_block){

    block_meta_data_t *return_block = NULL;

    assert(to_be_free_block->is_free == MM_FALSE);
    
    vm_page_t *hosting_page = 
        MM_GET_PAGE_FROM_META_BLOCK(to_be_free_block);

    vm_page_family_t *vm_page_family = hosting_page->pg_family;

    vm_page_family->total_memory_in_use_by_app -= 
        sizeof(block_meta_data_t) + to_be_free_block->block_size;
    
    to_be_free_block->is_free = MM_TRUE;
    
    return_block = to_be_free_block;

    block_meta_data_t *next_block = NEXT_META_BLOCK(to_be_free_block);

    if(next_block && next_block->is_free == MM_TRUE){
        /*Union two free blocks*/
        mm_union_free_blocks(to_be_free_block, next_block);
        return_block = to_be_free_block;
    }
    /*Check the previous block if it was free*/
    block_meta_data_t *prev_block = PREV_META_BLOCK(to_be_free_block);
    
    if(prev_block && prev_block->is_free){
        mm_union_free_blocks(prev_block, to_be_free_block);
        return_block = prev_block;
    }
    
    if(mm_is_vm_page_empty(hosting_page)){
        mm_vm_page_delete_and_free(hosting_page);
        return NULL;
    }
    mm_add_free_block_meta_data_to_free_block_list(
            hosting_page->pg_family, return_block);
    
    return return_block;
}

void
xfree(void *app_data){

    block_meta_data_t *block_meta_data = 
        (block_meta_data_t *)((char *)app_data - sizeof(block_meta_data_t));
   
    if(block_meta_data->is_free == MM_TRUE){
        printf("!Double Free detected\n");
        assert(0);
    }
    mm_free_blocks(block_meta_data);
}

vm_bool_t
mm_is_vm_page_empty(vm_page_t *vm_page){

    if(vm_page->block_meta_data.next_block == NULL && 
        vm_page->block_meta_data.prev_block == NULL &&
        vm_page->block_meta_data.is_free == MM_TRUE){

        return MM_TRUE;
    }
    return MM_FALSE;
}

void
mm_print_vm_page_details(vm_page_t *vm_page, uint32_t i){

    printf("\tPage Index : %u , address = %p\n", vm_page->page_index, vm_page);
    printf("\t\t next = %p, prev = %p\n", vm_page->next, vm_page->prev);
    printf("\t\t page family = %s\n", vm_page->pg_family->struct_name);

    uint32_t j = 0;
    block_meta_data_t *curr;
    ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page, curr){

        printf(ANSI_COLOR_YELLOW "\t\t\t%-14p Block %-3u %s  block_size = %-6u  "
                "offset = %-6u  prev = %-14p  next = %p\n"
                ANSI_COLOR_RESET, curr,
                j++, curr->is_free ? "F R E E D" : "ALLOCATED",
                curr->block_size, curr->offset, 
                curr->prev_block,
                curr->next_block);
    } ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page, curr);
}

void
mm_print_memory_usage(char *struct_name){

    uint32_t i = 0;
    vm_page_t *vm_page = NULL;
    vm_page_family_t *vm_page_family_curr; 
    uint32_t number_of_struct_families = 0;
    uint32_t total_memory_in_use_by_application = 0;
    uint32_t cumulative_vm_pages_claimed_from_kernel = 0;

    printf("\nPage Size = %zu Bytes\n", GB_SYSTEM_PAGE_SIZE);

    ITERATE_PAGE_FAMILIES_BEGIN(gb_heap_segment_start, vm_page_family_curr){
        
        if(struct_name){
            if(strncmp(struct_name, vm_page_family_curr->struct_name,
                        strlen(vm_page_family_curr->struct_name))){
                continue;
            }
        }

        number_of_struct_families++;

        printf(ANSI_COLOR_GREEN "vm_page_family : %s, struct size = %u\n" 
                ANSI_COLOR_RESET,
                vm_page_family_curr->struct_name,
                vm_page_family_curr->struct_size);
        printf(ANSI_COLOR_CYAN "\tApp Used Memory %uB, #Sys Calls %u\n"
                ANSI_COLOR_RESET,
                vm_page_family_curr->total_memory_in_use_by_app,
                vm_page_family_curr->\
                no_of_system_calls_to_alloc_dealloc_vm_pages);
        
        total_memory_in_use_by_application += 
            vm_page_family_curr->total_memory_in_use_by_app;

        i = 0;

        ITERATE_VM_PAGE_PER_FAMILY_BEGIN(vm_page_family_curr, vm_page){
      
            cumulative_vm_pages_claimed_from_kernel++;
            mm_print_vm_page_details(vm_page, i++);

        } ITERATE_VM_PAGE_PER_FAMILY_END(vm_page_family_curr, vm_page);
        printf("\n");
    } ITERATE_PAGE_FAMILIES_END(gb_heap_segment_start, vm_page_family_curr);

    printf(ANSI_COLOR_MAGENTA "\nTotal Applcation Memory Usage : %u Bytes\n"
        ANSI_COLOR_RESET, total_memory_in_use_by_application);

    printf(ANSI_COLOR_MAGENTA "# Of VM Pages in Use : %u (%lu Bytes).\n" \
        "Heap Segment Start ptr = %p, sbrk(0) = %p , gb_hsba = %p, diff = %lu\n" \
        ANSI_COLOR_RESET,
        cumulative_vm_pages_claimed_from_kernel, 
        GB_SYSTEM_PAGE_SIZE * cumulative_vm_pages_claimed_from_kernel,
        gb_heap_segment_start, sbrk(0), gb_hsba,
        (unsigned long)sbrk(0) - (unsigned long)gb_hsba);

    float memory_app_use_to_total_memory_ratio = 0.0;
    
    if(cumulative_vm_pages_claimed_from_kernel){
        memory_app_use_to_total_memory_ratio = 
        (float)(total_memory_in_use_by_application * 100)/\
        (float)(cumulative_vm_pages_claimed_from_kernel * GB_SYSTEM_PAGE_SIZE);
    }
    printf(ANSI_COLOR_MAGENTA "Memory In Use by Application = %f%%\n"
        ANSI_COLOR_RESET,
        memory_app_use_to_total_memory_ratio);

    printf("Total Memory being used by Memory Manager = %lu Bytes\n",
        ((cumulative_vm_pages_claimed_from_kernel *\
        GB_SYSTEM_PAGE_SIZE) + 
        (number_of_struct_families * sizeof(vm_page_family_t))));
}

void
mm_print_block_usage(){

    vm_page_t *vm_page_curr;
    vm_page_family_t *vm_page_family_curr;
    block_meta_data_t *block_meta_data_curr;
    uint32_t total_block_count, free_block_count,
             occupied_block_count;
    uint32_t application_memory_usage;

    ITERATE_PAGE_FAMILIES_BEGIN(gb_heap_segment_start, vm_page_family_curr){

        total_block_count = 0;
        free_block_count = 0;
        application_memory_usage = 0;
        occupied_block_count = 0;
        ITERATE_VM_PAGE_PER_FAMILY_BEGIN(vm_page_family_curr, vm_page_curr){

            ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page_curr, block_meta_data_curr){
        
                total_block_count++;
                
                /*Sanity Checks*/
                if(block_meta_data_curr->is_free == MM_FALSE){
                    assert(IS_GLTHREAD_LIST_EMPTY(&block_meta_data_curr->\
                                priority_thread_glue));
                }
                if(block_meta_data_curr->is_free == MM_TRUE){
                    assert(!IS_GLTHREAD_LIST_EMPTY(&block_meta_data_curr->\
                                priority_thread_glue));
                }

                if(block_meta_data_curr->is_free == MM_TRUE){
                    free_block_count++;
                }
                else{
                    application_memory_usage += 
                        block_meta_data_curr->block_size + \
                        sizeof(block_meta_data_t);
                    occupied_block_count++;
                }
            } ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page_curr, block_meta_data_curr);
        } ITERATE_VM_PAGE_PER_FAMILY_END(vm_page_family_curr, vm_page_curr);

        printf("%-20s   TBC : %-4u    FBC : %-4u    OBC : %-4u AppMemUsage : %u\n",
            vm_page_family_curr->struct_name, total_block_count,
            free_block_count, occupied_block_count, application_memory_usage);
    
    } ITERATE_PAGE_FAMILIES_END(gb_heap_segment_start, vm_page_family_curr); 
}
