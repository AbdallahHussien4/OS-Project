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
#define BUFFER_SIZE 10

// Semaphores Struct and functions
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}

int main(){
    // Create the initialization shared memory and semaphores
    key_t key_id;
    key_id = ftok("keyfile", 65);
    union Semun semun;
    // Semaphores
    int mutex = semget(key_id,1,0666 | IPC_CREAT);
    int full = semget(key_id+1,1,0666 | IPC_CREAT);
    int empty = semget(key_id+2,1,0666 | IPC_CREAT);
    if (mutex == -1 || full == -1 || empty == -1)
    {
        perror("Error in create semphores");
        exit(-1);
    }
    // intialize the semaphores
    FILE *fptr;
    fptr = fopen("init.txt","r");
    if(fptr == NULL){
        fptr = fopen("init.txt","a+");
        semun.val = 1;
        if (semctl(mutex, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl(mutex)");
            exit(-1);
        } 
        semun.val = 0;
        if (semctl(full, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl(full)");
            exit(-1);
        }
        semun.val = BUFFER_SIZE;
        if (semctl(empty, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl(empty)");
            exit(-1);
        }
    }
    fclose(fptr);
    // Shared memory 
    int bufferID = shmget(key_id,sizeof(int)*BUFFER_SIZE,0666 | IPC_CREAT);
    int addID = shmget(key_id+1,sizeof(int),0666 | IPC_CREAT);
    int remID = shmget(key_id+2,sizeof(int),0666 | IPC_CREAT);
    int numID = shmget(key_id+3,sizeof(int),0666 | IPC_CREAT);
    if (bufferID == -1 || addID == -1 || remID == -1 || numID == -1)
    {
        perror("Error in create the shared memory");
        exit(-1);
    }
    // Attach the shared memory to the program address
    int *bufferAddr = shmat(bufferID, (void *)0, 0);
    int *remAddr = shmat(addID, (void *)0, 0);
    int *addAddr = shmat(remID, (void *)0, 0);
    int *numAddr = shmat(numID, (void *)0, 0);

    printf("%d\n",bufferAddr[2]);
    // Main loop of the consumer
    printf("Enter the consumer loop \n");
    while(1){
        down(full);
        down(mutex);
        // if(numAddr[0]<0) exit(1);
        printf("item is consumed: %d\n",bufferAddr[remAddr[0]]);
        remAddr[0]= (remAddr[0]+1)%BUFFER_SIZE;
        numAddr[0]-=1;
        up(mutex);
        up(empty);
        sleep(3);
    }

    return 0;
}