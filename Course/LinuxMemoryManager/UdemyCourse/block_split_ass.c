#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

void
print_meta_block_stats(block_meta_data_t *first_block_meta_data);

static size_t SYSTEM_PAGE_SIZE = 0;

typedef enum{

    MM_FALSE,
    MM_TRUE
} vm_bool_t;

typedef struct block_meta_data_{

    vm_bool_t is_free;
    uint32_t block_size;
    uint32_t offset;    /*offset from the start of the page*/
    struct block_meta_data_ *prev_block;
    struct block_meta_data_ *next_block;
} block_meta_data_t;

/*Function to request VM page from kernel*/
static void *
mm_get_new_vm_page_from_kernel(int units){

    char *vm_page = mmap(
            0,
            units * SYSTEM_PAGE_SIZE,
            PROT_READ|PROT_WRITE|PROT_EXEC,
            MAP_ANON|MAP_PRIVATE,
            0, 0);

    if(vm_page == MAP_FAILED){
        printf("Error : VM Page allocation Failed\n");
        return NULL;
    }
    memset(vm_page, 0, units * SYSTEM_PAGE_SIZE);
    return (void *)vm_page;
}

static void
split_blocks(block_meta_data_t *block_meta_data){

/*
 This function fragments the VM page of size 4096B 
 as per the below diagram
 +------------------+
 |                  |
 |                  |
 |                  |
 |      DB4         |
 |                  |
 |                  |
 |                  |
 +------------------+
 |      MB4         |
 | NF, 3264, 804    |
 +------------------+
 |                  |
 |      DB3         |
 |                  |
 +------------------+
 |      MB3         |
 |F, 220, 556       |
 +------------------+
 |                  |
 |      DB2         |
 |                  |
 |                  |
 +------------------+
 |     MB2          |
 | NF, 300, 228     |
 +------------------+
 |                  |
 |     DB1          |
 |                  |
 |                  |
 |                  |
 +------------------+
 |     MB1          |
 | F, 200, 0        |
 +------------------+

*/

    block_meta_data_t *MB1 = block_meta_data;
    MB1->is_free = MM_TRUE;
    MB1->block_size = 200;
    MB1->offset = 0;

    block_meta_data_t *MB2 = NEXT_META_BLOCK_BY_SIZE(MB1);
    MB2->is_free = MM_FALSE;
    MB2->block_size = 300;
    MB2->offset = MB1->offset + sizeof(block_meta_data_t) + MB1->block_size;

    mm_bind_blocks_for_allocation(MB1, MB2);

    block_meta_data_t *MB3 = NEXT_META_BLOCK_BY_SIZE(MB2);
    MB3->is_free = MM_FALSE;
    MB3->block_size = 220;
    MB3->offset = MB2->offset + sizeof(block_meta_data_t) + MB2->block_size;

    mm_bind_blocks_for_allocation(MB2, MB3);

    block_meta_data_t *MB4 = NEXT_META_BLOCK_BY_SIZE(MB3);
    MB4->is_free = MM_FALSE;
    MB4->block_size = 3264;
    MB4->offset = MB3->offset + sizeof(block_meta_data_t) + MB3->block_size;

    mm_bind_blocks_for_allocation(MB3, MB4);

}

static void
print_one_meta_block_stats(block_meta_data_t *block_meta_data){

    printf("is_free = %s  size = %-5d  offset = %-5d  prev = %p  next = %p\n",
            block_meta_data->is_free ? "TRUE", "FALSE",
            block_meta_data->block_size, 
            block_meta_data->offset, 
            block_meta_data->prev_block,
            block_meta_data->next_block);

}

static void
print_all_meta_blocks(block_meta_data_t *first_meta_block){


    while(first_meta_block){

        print_one_meta_block_stats(block_meta_data);
        first_meta_block = first_meta_block->next_block;
    }
}


int
main(int argc, char **argv){
    
    SYSTEM_PAGE_SIZE = getpagesize();

    void *vm_page = mm_get_new_vm_page_from_kernel(1);
    if(!vm_page){
        printf("Error : Page allocation failed\n");
        return 0;
    }

    /*Covert VM page into 1 single Free data block*/
    block_meta_data_t *first_block_meta_data = 
        (block_meta_data_t *)vm_page;

    first_block_meta_data->is_free = MM_TRUE;
    first_block_meta_data->block_size = SYSTEM_PAGE_SIZE - \
        sizeof(block_meta_data_t);
    first_block_meta_data->offset = 0;
    first_block_meta_data->prev_block = 0;
    first_block_meta_data->next_block = 0;
    split_blocks(first_block_meta_data);
    print_all_meta_blocks(first_block_meta_data);
    return 0;
}
