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
    if(!first_vm_page_family)
        assert(0);
}

vm_page_t *
allocate_vm_page(){

    vm_page_t *vm_page = calloc(1, SYSTEM_PAGE_SIZE);
    vm_page->block_meta_data.is_free = MM_TRUE;
    vm_page->block_meta_data.block_size = 
        (SYSTEM_PAGE_SIZE - sizeof(block_meta_data_t));
    vm_page->block_meta_data.offset = 0;
    vm_page->block_meta_data.next = NULL;
    vm_page->block_meta_data.prev = NULL;
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
mm_add_free_block_data_to_free_block_list(
        glthread_t *list_head, block_meta_data_t *free_block){

    assert(free_block->is_free == MM_TRUE);
    glthread_priority_insert(list_head, &free_block->priority_thread_glue,
            free_blocks_comparison_function,
            (size_t)&(((block_meta_data_t *)0)->priority_thread_glue));
}

static vm_page_t *
mm_get_page_satisfying_request(
        vm_page_family_t *vm_page_family,
        uint32_t req_size, 
        block_meta_data_t **block_meta_data/*O/P*/){

    return NULL;    
}

static void *
mm_page_allocate_memory(vm_page_t * vm_page, 
            uint32_t req_size){

    return NULL;
}

static vm_page_t *
mm_family_new_page_add(vm_page_family_t *vm_page_family){

    return NULL;
}


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
    
    if(units * pg_family->struct_size > SYSTEM_PAGE_SIZE){
        
        printf("Error : Memory Requested Exceeds Page Size\n");
        return NULL;
    }

    if(!pg_family->first_page){
        pg_family->first_page = allocate_vm_page();
        pg_family->first_page->block_meta_data.is_free = MM_FALSE;
        pg_family->first_page->block_meta_data.block_size = 
            units * pg_family->struct_size;
        pg_family->first_page->block_meta_data.next = NULL;
        pg_family->first_page->block_meta_data.prev = NULL;
        mm_add_free_block_data_to_free_block_list(
            &pg_family->free_block_priority_list_head,
            &pg_family->first_page->block_meta_data);
        return (void *)pg_family->first_page->page_memory;
    }
    
    /*Find the page which can satisfy the request*/
    block_meta_data_t *free_block_meta_data;
    vm_page_t *vm_page_curr = mm_get_page_satisfying_request(
                        pg_family, units * pg_family->struct_size,
                        &free_block_meta_data);
    if(!vm_page_curr){
        /*Create a new page*/
        vm_page_curr = mm_family_new_page_add(pg_family);
        result = mm_page_allocate_memory(vm_page_curr, 
            units * pg_family->struct_size);
        return result;
    }
    return  (void *)(free_block_meta_data + 1);
}



