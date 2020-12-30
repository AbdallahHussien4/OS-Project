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

typedef short bool;
#define true 1
#define false 1

#define SHKEY 300


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================

// buffer used to send processes data between process generator and schedular
struct msgbuffer
{
    long mtype;
    long ArrivalTime;
    long RunTime;
    long Priority;
    long Id;
};

struct process
{
    long ArrivalTime;
    long RunTime;
    long Priority;
    long Id;
    long WaitingTime;
    long RemainingTime;
    struct process* next; 
};

// typedef struct Node{
//     struct process P;
//     struct Node * Node;
// };

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
        printf("Empty Queue");
        return;
    }
    while(start){
        printf("%ld\n", start->Priority);
        start = start->next;
    }
    printf("=============\n");
    return;
};



// Removes the element with the 
// highest priority form the list 
struct process * pop(Queue * q) 
{ 
    struct process* temp = q->head; 
    (q->head) = (q->head)->next; 
    return temp; 
} 

// Function to push according to priority 
void push(Queue * q, struct process * p, int Algorithm) 
{ 
    struct process* start = (q->head); 
  
    // Create new Node 
    struct process* temp = newNode(p); 
  
    // Special Case: The head of list has lesser 
    // priority than new node. So insert new 
    // node before head node and change head node. 
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
            // Insert New Node before head 
            temp->next = q->head; 
            (q->head) = temp; 
        } 
        else
        { 
            // Traverse the list and find a 
            // position to insert new node 
            while (start->next != NULL && start->next->Priority < p->Priority)
            { 
                start = start->next; 
            } 
    
            // Either at the ends of the list 
            // or at required position 
            temp->next = start->next; 
            start->next = temp; 
        }
        break;
    case 2:
        if ((q->head)->RemainingTime > p->RemainingTime) 
        { 
            // Insert New Node before head 
            temp->next = q->head; 
            (q->head) = temp; 
        } 
        else
        { 
            // Traverse the list and find a 
            // position to insert new node 
            while (start->next != NULL && start->next->RemainingTime < p->RemainingTime)
            { 
                start = start->next; 
            } 
    
            // Either at the ends of the list 
            // or at required position 
            temp->next = start->next; 
            start->next = temp; 
        }
        break;
    default:
        while (start->next != NULL)
        { 
            start = start->next; 
        } 

        // Either at the ends of the list 
        // or at required position 
        temp->next = start->next; 
        start->next = temp; 
        break;        
    } 
} 
