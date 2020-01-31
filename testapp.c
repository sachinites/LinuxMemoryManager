#include <stdio.h>
#include "mm.h"

typedef struct emp_ {

    char name[32];
    uint32_t emp_id;
} emp_t;

int
main(int argc, char **argv){

    mm_init();
    mm_instantiate_new_page_family("emp_t", sizeof(emp_t));

    emp_t *emp = xcalloc("emp_t", 1);
    mm_print_memory_usage();
    xfree(emp);
    mm_print_memory_usage();
    return 0;
}
