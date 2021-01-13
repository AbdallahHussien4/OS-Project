#include "headers.h"


int main(){
    Memory * q = initMemory();
    struct sector * s = allocate(q, 256);
    for(int i = 0; i < 6; i++)
        printSector(q->head[i]);
    
    int p = deallocate(q, s);
    fprintf(stderr, "%d\n", p);
    return 0;
}