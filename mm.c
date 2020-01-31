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

#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h> /*for getpagesize*/

static vm_page_family_t *first_vm_page_family = NULL;
static size_t SYSTEM_PAGE_SIZE = 0;

void
mm_init(){

    SYSTEM_PAGE_SIZE = getpagesize();
}

static inline uint32_t
mm_max_page_allocatable_memory(){

    return (uint32_t)
        (SYSTEM_PAGE_SIZE - offset_of(vm_page_t, page_memory));
}

#define MAX_PAGE_ALLOCATABLE_MEMORY \
    (mm_max_page_allocatable_memory())

/*Return a fresh new virtual page*/
vm_page_t *
allocate_vm_page(){

    vm_page_t *vm_page = calloc(1, SYSTEM_PAGE_SIZE);
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
    return vm_page;
}


void
mm_instantiate_new_page_family(
    char *struct_name,
    uint32_t struct_size){

    if(struct_size > SYSTEM_PAGE_SIZE){
        printf("Error : %s() Structure Size exceeds system page size\n",
            __FUNCTION__);
        return;
    }

    if(!first_vm_page_family){
        first_vm_page_family = calloc(1, sizeof(vm_page_family_t));
        strncpy(first_vm_page_family->struct_name, struct_name,
            MM_MAX_STRUCT_NAME);
        first_vm_page_family->struct_size = struct_size;
        first_vm_page_family->first_page = NULL;
        first_vm_page_family->next = NULL;
        first_vm_page_family->prev = NULL;
        init_glthread(&first_vm_page_family->free_block_priority_list_head);
        return;
    }

    vm_page_family_t *vm_page_family_curr;

    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_family, vm_page_family_curr){

        if(strncmp(vm_page_family_curr->struct_name, 
            struct_name, 
            MM_MAX_STRUCT_NAME) != 0){
            
            continue;
        }
        /*Page family already exists*/
        assert(0);
    } ITERATE_PAGE_FAMILIES_END(first_vm_page_family, vm_page_family_curr);

    /*Page family do not exist, create a new one*/
    vm_page_family_curr = calloc(1, sizeof(vm_page_family_t));
    strncpy(vm_page_family_curr->struct_name, struct_name,
            MM_MAX_STRUCT_NAME);
    vm_page_family_curr->struct_size = struct_size;
    vm_page_family_curr->first_page = NULL;
    init_glthread(&first_vm_page_family->free_block_priority_list_head);

    /*Add new page family to the list of Page families*/
    vm_page_family_curr->next = first_vm_page_family;
    first_vm_page_family->prev = vm_page_family_curr;
    first_vm_page_family = vm_page_family_curr;
}

vm_page_family_t *
lookup_page_family_by_name(char *struct_name){

    vm_page_family_t *vm_page_family_curr;
    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_family, vm_page_family_curr){

        if(strncmp(vm_page_family_curr->struct_name,
            struct_name,
            MM_MAX_STRUCT_NAME) == 0){

            return vm_page_family_curr;
        }
    } ITERATE_PAGE_FAMILIES_END(first_vm_page_family, vm_page_family_curr);
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
            (size_t)&(((block_meta_data_t *)0)->priority_thread_glue));
}

static vm_page_t *
mm_family_new_page_add(vm_page_family_t *vm_page_family){

    vm_page_t *vm_page = allocate_vm_page();

    /* The new page is like one free block, add it to the
     * free block list*/
    mm_add_free_block_meta_data_to_free_block_list(
        vm_page_family, &vm_page->block_meta_data);

    /*Add the page to the list of pages maintained by page family*/
    if(!vm_page_family->first_page){
        vm_page_family->first_page = vm_page;
        vm_page->pg_family = vm_page_family;
        return vm_page;
    }
    vm_page->pg_family = vm_page_family;
    /* Add this new virtual Memory Page to front of the
     * list managed by page family*/
    vm_page->next = vm_page_family->first_page;
    vm_page_family->first_page->prev = vm_page;
    vm_page_family->first_page = vm_page;
    return vm_page;
}

/*Fn to mark block_meta_data as being Allocated for
 * 'size' bytes of application data*/
static void
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

    /*No need to do anything else if this block is completely used
     * to satisfy memory request*/
    if(!remaining_size)
        return;

    /*If the remaining memory chunk in this free block is unusable
     * because of fragmentation - however this should not be possible
     * except the boundry condition*/
    if(remaining_size < sizeof(block_meta_data_t)){
        printf("Warning : Memory Unusable : No enough space to be used"
            " by Meta block\n");
        return;
    }

    block_meta_data_t *next_block_meta_data = NULL;

    next_block_meta_data = NEXT_META_BLOCK_BY_SIZE(block_meta_data);

    next_block_meta_data->is_free = MM_TRUE;

    next_block_meta_data->block_size = 
        remaining_size - sizeof(block_meta_data_t);

    next_block_meta_data->offset = block_meta_data->offset + 
        sizeof(block_meta_data) + block_meta_data->block_size;

    init_glthread(&next_block_meta_data->priority_thread_glue); 
    mm_bind_blocks(block_meta_data, next_block_meta_data);
    mm_add_free_block_meta_data_to_free_block_list(
            vm_page_family, next_block_meta_data);
}

static vm_page_t *
mm_get_page_satisfying_request(
        vm_page_family_t *vm_page_family,
        uint32_t req_size, 
        block_meta_data_t **block_meta_data/*O/P*/){

    vm_page_t *vm_page = NULL;

    block_meta_data_t *biggest_block_meta_data = 
        mm_get_biggest_free_block_page_family(vm_page_family); 

    if(!biggest_block_meta_data || 
        biggest_block_meta_data->block_size < req_size){

        /*Time to add a new page to Page family to satisfy the request*/
        vm_page = mm_family_new_page_add(vm_page_family);
        /*Allocate the free block from this page now*/
        mm_allocate_free_block(vm_page_family, 
            &vm_page->block_meta_data, req_size);
        *block_meta_data = &vm_page->block_meta_data;
        return vm_page;
    }
    /*The biggest block meta data can satisfy the request*/
    mm_allocate_free_block(vm_page_family, 
        biggest_block_meta_data, req_size);
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
        mm_allocate_free_block(pg_family, 
            &pg_family->first_page->block_meta_data, 
            units * pg_family->struct_size);
        return (void *)pg_family->first_page->page_memory;
    }
    
    /*Find the page which can satisfy the request*/
    block_meta_data_t *free_block_meta_data;
    vm_page_t *vm_page_curr = mm_get_page_satisfying_request(
                        pg_family, units * pg_family->struct_size,
                        &free_block_meta_data);
    return  (void *)(free_block_meta_data + 1);
}

static void
mm_union_free_blocks(block_meta_data_t *first,
        block_meta_data_t *second){

    assert(first->is_free == MM_TRUE &&
        second->is_free == MM_TRUE);

    block_meta_data_t *third_block = 
        NEXT_META_BLOCK(second);
        
    first->block_size += sizeof(block_meta_data_t) +
            second->block_size;
    remove_glthread(&first->priority_thread_glue);
    remove_glthread(&second->priority_thread_glue);
    if(third_block){
        mm_bind_blocks(first, third_block);
    }
}

static void
mm_vm_page_delete_and_free(
        vm_page_t *vm_page){

    vm_page_family_t *vm_page_family = 
        vm_page->pg_family;

    assert(vm_page_family->first_page);

    if(vm_page_family->first_page == vm_page){
        vm_page_family->first_page = vm_page->next;
        if(vm_page->next)
            vm_page->next->prev = NULL;
        free(vm_page);
        return;
    }

    vm_page->next->prev = vm_page->prev;
    vm_page->prev->next = vm_page->next;
    free(vm_page);
}

static block_meta_data_t *
mm_free_blocks(block_meta_data_t *to_be_free_block){

    block_meta_data_t *return_block = NULL;

    assert(to_be_free_block->is_free == MM_FALSE);
    
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
    
    vm_page_t *hosting_page = MM_GET_PAGE_FROM_META_BLOCK(return_block);

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
    
    assert(block_meta_data->is_free == MM_FALSE);
    mm_free_blocks(block_meta_data);
}

vm_bool_t
mm_is_vm_page_empty(vm_page_t *vm_page){

    if(vm_page->block_meta_data.next_block == NULL && 
        vm_page->block_meta_data.prev_block == NULL &&
        vm_page->block_meta_data.is_free == MM_TRUE &&
        vm_page->block_meta_data.block_size == MAX_PAGE_ALLOCATABLE_MEMORY){

        return MM_TRUE;
    }
    return MM_FALSE;
}

void
mm_print_vm_page_details(vm_page_t *vm_page, uint32_t i){

    printf("\tPage %u. \n", i);
    printf("\t\t next = %p, prev = %p\n", vm_page->next, vm_page->prev);
    printf("\t\t page family = %s\n", vm_page->pg_family->struct_name);

    uint32_t j = 0;
    block_meta_data_t *curr;
    ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page, curr){

        printf("\t\t\t Block %-3u %s  block_size = %-6u  "
                "offset = %-6u  prev = %-14p  next = %p\n", 
                j++, curr->is_free ? "F R E E D" : "ALLOCATED",
                curr->block_size, curr->offset, 
                curr->prev_block,
                curr->next_block);
    } ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page, curr);
}

void
mm_print_memory_usage(){

    vm_page_t *vm_page = NULL;
    vm_page_family_t *vm_page_family_curr; 
    printf("\nPage Size = %zu\n", SYSTEM_PAGE_SIZE);
    printf("Page Families Registered : \n");
    uint32_t i = 0;

    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_family, vm_page_family_curr){

        printf("vm_page_family : %s, struct size = %u\n", 
                vm_page_family_curr->struct_name,
                vm_page_family_curr->struct_size);

        ITERATE_VM_PAGE_BEGIN(vm_page_family_curr, vm_page){
            mm_print_vm_page_details(vm_page, i++);
        } ITERATE_VM_PAGE_END(vm_page_family_curr, vm_page);
        printf("\n");
    } ITERATE_PAGE_FAMILIES_END(first_vm_page_family, vm_page_family_curr)
}
