#include "uapi_mm.h"

typedef struct emp_ {

    char name[32];
    uint32_t emp_id;
} emp_t;

typedef struct student_ {

    char name[32];
    uint32_t rollno;
    uint32_t marks_phys;
    uint32_t marks_chem;
    uint32_t marks_maths;
    struct student_ *next;
} student_t;

int
main(int argc, char **argv){

    mm_init();
    MM_REG_STRUCT(emp_t);
    MM_REG_STRUCT(student_t);
    mm_print_registered_page_families();
    return 0; 
}
