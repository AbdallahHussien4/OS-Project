#include "headers.h"


int main(){
    Memory * q = initMemory();
    struct sector * s = allocate(q, 16);
    s = allocate(q, 64);
    s = allocate(q, 16);
    // fprintf(stderr, "start: %d\tend: %d\n", s->s, s->e);
    for(int i = 0; i < 6; i++)
        printSector(q->head[i]);
    return 0;
}