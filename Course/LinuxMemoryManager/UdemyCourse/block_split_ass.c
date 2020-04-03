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

    print_meta_block_stats(first_block_meta_data);

    return 0;
}
