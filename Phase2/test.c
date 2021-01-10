#include "headers.h"


int main(){
    MemoryQueue * q = initMemory();
    struct sector s;
    s.s = 0;
    s.e = 15;
    struct sector * p = pushSector(q, &s, true);
    if(p)
    {
        fprintf(stderr, "MERGED: %d %d\n", p->s, p->e);
    }
    else
    {
        fprintf(stderr, "NULL\n");
    }
    s.s = 16;
    s.e = 31;
    p = pushSector(q, &s, false);
    if(p)
    {
        fprintf(stderr, "MERGED: %d %d\n", p->s, p->e);
    }
    else
    {
        fprintf(stderr, "NULL\n");
    }
    s.s = 48;
    s.e = 63;
    p = pushSector(q, &s, true);
    if(p)
    {
        fprintf(stderr, "MERGED: %d %d\n", p->s, p->e);
    }
    else
    {
        fprintf(stderr, "NULL\n");
    }
    s.s = 32;
    s.e = 47;
    p = pushSector(q, &s, true);
    if(p)
    {
        fprintf(stderr, "MERGED: %d %d\n", p->s, p->e);
    }
    else
    {
        fprintf(stderr, "NULL\n");
    }

    printSector(q);
    return 0;
}