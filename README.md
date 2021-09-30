# LinuxMemoryManager
This projects is regarding designing own Heap memory manager which will manage the process's memory requirement. Memory manager will get rid Or minimize the internal and external fragmentation problems, and boost performance of the process by preventing unnecessary system calls for allocating/releasing the memory. Not only that, LMM can display the user with the statistics regarding the memory usage by each structure of the process. From this stats, application memory usage can be analyzed and can provide hints to further optimize the memory requirements of the process. Memory leak can also be discovered using LMM.

LMM request and release memmory from kernel space on behalf of application in units of Virtual Memory Pages. It used mmap() and munmap() system calls for this purpose. It caches the VM pages and use it as reservoir for future memory requests issued by the application, until the VM page is fully exhausted. VM page is released back to kernel if application has freed enough Memory such that VM page has no region occupied/allocated to the application for use.


Future Enhancement : 
A GUI tool can be developed which can fetch the Memory usage stats from LMM and display graphically in real time.


Algorithms used :
= = = = = = = = = 
Algorithms for Block Splitting and Merging
Doubly linked list for maintaining free and allocated blocks
Largest fit Algorithms using priority Queue Data Structure for allocating memory to the process


Compilations:
gcc -g -c testapp.c -o testapp.o
gcc -g -c mm.c -o mm.o
gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
gcc -g testapp.o mm.o gluethread/glthread.o -o exe
