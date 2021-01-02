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
    int ArrivalTime;
    int RunTime;
    int Priority;
    int Id;
};

struct process
{
    bool forked;
    int processId;
    int lastTime;
    int ArrivalTime;
    int RunTime;
    int Priority;
    int Id;
    int WaitingTime;
    int RemainingTime;
    struct process* next; 
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

struct process * newNode(struct process * p) 
{ 
    struct process * temp = (struct process *)malloc(sizeof(struct process)); 
    temp->ArrivalTime = p->ArrivalTime;
    temp->lastTime = p->lastTime;
    temp->processId = p->processId;
    temp->RunTime = p->RunTime;
    temp->Priority = p->Priority;
    temp->Id = p->Id;
    temp->WaitingTime = p->WaitingTime;
    temp->RemainingTime = p->RemainingTime;
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
        printf("%d\n", start->Priority);
        start = start->next;
    }
    printf("=============\n");
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
