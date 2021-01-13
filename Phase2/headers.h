#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================

// buffer used to send processes data between process generator and scheduler
struct msgbuffer
{
    int mtype;
    int memory;
    int ArrivalTime;
    int RunTime;
    int Priority;
    int Id;
};

struct process
{
    int processId;
    int memory;
    int lastTime;
    int ArrivalTime;
    int RunTime;
    int Priority;
    int Id;
    int WaitingTime;
    int RemainingTime;
    struct process* next; 
    struct sector * Sector;
};

struct sector
{
    short s;
    short e;
    struct sector * next; 
};

int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}






// ===============================================
// =============== Priority Queue ================
// ===============================================
struct Queue{
    struct process * head;
    long size;
};

typedef struct Queue Queue;

Queue * initQueue()
{
    Queue * q = (Queue *)malloc(sizeof(Queue));
    q->head = NULL;
    q->size = 0;
    return q;
}

struct process * newNode(struct process * p) 
{ 
    struct process * temp = (struct process *)malloc(sizeof(struct process)); 
    temp->ArrivalTime = p->ArrivalTime;
    temp->RunTime = p->RunTime;
    temp->Priority = p->Priority;
    temp->Id = p->Id;
    temp->WaitingTime = p->WaitingTime;
    temp->RemainingTime = p->RemainingTime;
    temp->lastTime = p->lastTime;
    temp->processId = p->processId;
    temp->memory = p->memory;
    temp->next = NULL;
    
    return temp; 
}

void printQueue(Queue * q){
    struct process* start = (q->head); 
    if(start == NULL)
    {
        printf("Empty Queue\n");
        return;
    }
    while(start){
        fprintf(stderr,"ID: %d, M:%d\n", start->Id, start->memory);
        start = start->next;
    }
    printf("========\n");
    return;
};


struct process * pop(Queue * q) 
{ 
    struct process * temp = q->head; 
    q->head = q->head->next; 
    return temp; 
} 

void push(Queue * q, struct process * p, int Algorithm) 
{ 
    struct process* start = (q->head); 
    struct process* temp = newNode(p); 

    if((q->head) == NULL)
    {
        q->head = temp;
        return;
    }
    switch (Algorithm)
    {
        case 1:
            if ((q->head)->Priority > p->Priority) 
            { 
                temp->next = q->head; 
                (q->head) = temp; 
            } 
            else
            { 
                while (start->next != NULL && start->next->Priority < p->Priority)
                { 
                    start = start->next; 
                } 
        
                temp->next = start->next; 
                start->next = temp; 
            }
            break;
        case 2:
            if ((q->head)->RemainingTime > p->RemainingTime) 
            { 
                temp->next = q->head; 
                (q->head) = temp; 
            } 
            else
            { 
                while (start->next != NULL && start->next->RemainingTime < p->RemainingTime)
                { 
                    start = start->next; 
                } 
                temp->next = start->next; 
                start->next = temp; 
            }
            break;
        default:
            while (start->next != NULL)
            { 
                start = start->next; 
            } 
            temp->next = start->next; 
            start->next = temp; 
            break;        
    } 
} 

// ===============================================
// =============== Array =========================
// ===============================================

typedef struct {
  float *array;
  size_t used;
  size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
  a->array = malloc(initialSize * sizeof(float));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, float element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only after the array has been accessed.
  // Therefore a->used can go up to a->size 
  if (a->used == a->size) {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(float));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}



// ===============================================
// ==================== MEMORY ===================
// ===============================================
int nextPowerOf2(unsigned int n) 
{ 
    unsigned count = 0; 
    if (n && !(n & (n - 1))) 
        return n; 
    
    while( n != 0) 
    { 
        n >>= 1; 
        count += 1; 
    } 
    
    return 1 << count; 
} 
struct MemoryQueue{
    struct sector * head;
    long size;
};

typedef struct MemoryQueue MemoryQueue;
MemoryQueue * initMemoryQ()
{
    MemoryQueue * q = (MemoryQueue *)malloc(sizeof(MemoryQueue));
    q->head = NULL;
    q->size = 0;
    return q;
}

struct sector * newSector(struct sector * p) 
{ 
    struct sector * temp = (struct sector *)malloc(sizeof(struct sector)); 
    temp->s = p->s;
    temp->e = p->e;
    temp->next = NULL;
    
    return temp; 
}

void printSector(MemoryQueue * q){
    struct sector* start = (q->head); 
    if(start == NULL)
    {
        printf("Empty Queue\n");
        return;
    }
    while(start){
        fprintf(stderr,"S: %d, E: %d\n", start->s, start->e);
        start = start->next;
    }
    printf("========\n");
    return;
};

struct sector * popSector(MemoryQueue * q) 
{ 
    struct sector * temp = q->head; 
    q->head = q->head->next; 
    return temp; 
} 

struct sector * pushSector(MemoryQueue * q, struct sector * p, bool merge) 
{ 
    struct sector* start = (q->head);
    struct sector* previous = (q->head); 
    struct sector* temp = newSector(p); 

    if((q->head) == NULL)
    {
        q->head = temp;
        return NULL;
    }
    if ((q->head)->s > p->s) 
    { 
        temp->next = q->head; 
        (q->head) = temp; 
    } 
    else
    { 
        while (start->next != NULL && start->next->s < p->s)
        { 
            previous = start;
            start = start->next;
        } 

        temp->next = start->next; 
        start->next = temp; 
    }
    if(merge && ((p->e - p->s) < 255))
    {
        if(temp->next && !((temp->s / (temp->e - temp->s))%2))
        {
            if(temp->next->s == temp->e + 1)
            {
                struct sector * t = (struct sector *)malloc(sizeof(struct sector));
                t->s = temp->s;
                t->e = temp->next->e;
                if(temp == q->head)
                    q->head = temp->next->next;
                else
                    start->next = temp->next->next;
                return t;
            }
        }

        if(start->next && !((start->s / (temp->e - temp->s))%2))
        {
            if(temp->s == start->e + 1)
            {
                struct sector * t = (struct sector *)malloc(sizeof(struct sector));
                t->s = start->s;
                t->e = temp->e;
                if(start == q->head)
                    q->head = temp->next;
                else
                    previous->next = temp->next;
                return t;
            }
        }
    }
    return NULL;
}


struct Memory{
    struct MemoryQueue * head[6];
};

typedef struct Memory Memory;

Memory * initMemory()
{
    Memory * q = (Memory *)malloc(sizeof(Memory));
    for(int i = 0; i<6; i++)
        q->head[i] = initMemoryQ();
    
    struct sector * s = (struct sector *)malloc(sizeof(struct sector));
    s->s = 0;
    s->e = 255;
    pushSector(q->head[5], s, false);
    s->s = 256;
    s->e = 511;
    pushSector(q->head[5], s, false);
    s->s = 512;
    s->e = 767;
    pushSector(q->head[5], s, false);
    s->s = 768;
    s->e = 1024;
    pushSector(q->head[5], s, false);

    return q;
}

struct sector * allocate(Memory * m, int size)
{
    int real_size = nextPowerOf2(size);

    int index = (log(real_size) / log(2)) - 3 ;
    while((index < 6) && (m->head[index]->head == NULL))
        index++;
    if(index == 6)
        return NULL;

    struct sector * s = popSector(m->head[index]);
    struct sector * temp = (struct sector *)malloc(sizeof(struct sector));
    while(((s->e + 1) - s->s) > real_size)
    {
        temp->s = s->s + (s->e - s->s)/2 + 1;
        temp->e = s->e;
        index--;
        pushSector(m->head[index], temp, false);
        s->e = s->s + (s->e - s->s)/2;
    }
    return s;
}

int deallocate(Memory * m, struct sector *s)
{   
    int index = (log(s->e + 1 - s->s) / log(2)) - 3 ;
    struct sector * pushed = pushSector(m->head[index], s, true);
    while((pushed != NULL))
    {
        index++;
        if(index < 5)
            pushed = pushSector(m->head[index], pushed, true);
        else
            pushed = pushSector(m->head[index], pushed, false);
        
    }
    if(pushed)
        return (pushed->e - pushed->s +1);
    else 
        return 256;
}