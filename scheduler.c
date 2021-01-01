#include "headers.h"

void receiveProcess(int signum);
void lastProcess(int signum);

int Algorithm, rec_val, stat_loc, msgq_id, pid;
Queue Processes;
bool finished = false;

int main(int argc, char * argv[])
{
    signal(SIGUSR1, receiveProcess);
    signal(SIGUSR2, lastProcess);
    Algorithm = atoi(argv[1]);
    // int n = atoi(argv[2]);
    // printf("%d\n", n);
    printf("Algorithm Number is %d\n",Algorithm);
    initClk();
    
    key_t key_id;
    

    key_id = ftok("gen_schdlr_com", 65);               
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); 

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    //printf("Message Queue ID = %d\n", msgq_id);
    switch(Algorithm)
    {
        case 1:
            while(!finished || (Processes.head != NULL))
            {
                if(Processes.head != NULL)
                {
                    struct process * CurrentProcess = pop(&Processes);
                    CurrentProcess->WaitingTime = getClk() - CurrentProcess->ArrivalTime;
                    pid = fork();
                    
                    if(!pid)
                    {
                        printf("Process with id %d has started at %d\n", CurrentProcess->Id, getClk());
                        char number[6];
                        snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                        execl("process.out", "process.out", number, NULL);
                    }
                    waitpid(pid, &stat_loc, 0);
                    printf("Process with id %d has finished at %d and waited for %d\n=======================\n", CurrentProcess->Id, getClk(), CurrentProcess->WaitingTime);
                    free(CurrentProcess);

                }
            }
            break;            
        case 2:
            break;
        default:
            break;

    }


    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    destroyClk(false);
}

void receiveProcess(int signum){
    signal(SIGUSR1, receiveProcess);
    struct msgbuffer message;
    struct process receivedProcess;
    do
    {
        rec_val = msgrcv(msgq_id, &message, sizeof(message), 0, IPC_NOWAIT);
        if(rec_val!=-1)
        {
            receivedProcess.Id = message.Id;
            receivedProcess.ArrivalTime = message.ArrivalTime;
            receivedProcess.RunTime = message.RunTime;
            receivedProcess.Priority = message.Priority;
            receivedProcess.WaitingTime = 0;
            receivedProcess.RemainingTime = message.RunTime;
            receivedProcess.next = NULL;
            // printf("%d %d %d %d %d %d\n", receivedProcess.Id, receivedProcess.ArrivalTime, receivedProcess.Priority, receivedProcess.RunTime, receivedProcess.RemainingTime, receivedProcess.WaitingTime);
            push(&Processes, &receivedProcess, Algorithm);
            // printQueue(&Processes);
        }
    } while(rec_val!=-1);
}

void lastProcess(int signum){
    printf("******************\n%d\n*********************\n",getClk());
    finished = true;
}