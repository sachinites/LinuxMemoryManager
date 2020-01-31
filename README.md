# LinuxMemoryManager
This projects is regarding designing own memory manager which will manage the memory - allocation/de-allocation for a process. Memory manager will get rid of internal and external fragmentation problems, and boot performance of the process by preventing unnecessary context switching of a process whenever process request memory.

Compilations:
gcc -g -c testapp.c -o testapp.o
gcc -g -c mm.c -o mm.o
gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
gcc -g testapp.o mm.o gluethread/glthread.o -o exe
