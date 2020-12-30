#include "headers.h"



int main(int argc, char * argv[])
{
    int Algorithm = atoi(argv[1]);
    printf("Algorithm Number is %d\n",Algorithm);
    initClk();
    Queue Processes;
    key_t key_id;
    int rec_val, msgq_id;

    key_id = ftok("gen_schdlr_com", 65);               
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); 

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    //printf("Message Queue ID = %d\n", msgq_id);

    struct msgbuffer message;
    struct process receivedProcess;
    while(1)
    {
    
        rec_val = msgrcv(msgq_id, &message, sizeof(message), 0, 0);

        if (rec_val == -1)
            perror("Error in receive");
        else
        {
            receivedProcess.Id = message.Id;
            receivedProcess.ArrivalTime = message.ArrivalTime;
            receivedProcess.RunTime = message.RunTime;
            receivedProcess.Priority = message.Priority;
            receivedProcess.WaitingTime = 0;
            receivedProcess.RemainingTime = message.RunTime;
            receivedProcess.next = NULL;
            // printf("%ld %ld %ld %ld %ld %ld\n", receivedProcess.Id, receivedProcess.ArrivalTime, receivedProcess.Priority, receivedProcess.RunTime, receivedProcess.RemainingTime, receivedProcess.WaitingTime);
            push(&Processes, &receivedProcess, Algorithm);
        }
        //printf("%ld %ld %ld %ld %ld %ld\n", receivedProcess.Id, receivedProcess.ArrivalTime, receivedProcess.Priority, receivedProcess.RunTime, receivedProcess.RemainingTime, receivedProcess.WaitingTime);
        printQueue(&Processes);
 
    }



    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    
    destroyClk(true);
}
