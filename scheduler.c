#include "headers.h"

void receiveProcess(int signum);
void lastProcess(int signum);
void HPF();
void SRTN();
void RR(int Quantum);
int Algorithm, rec_val, stat_loc, msgq_id, pid;
Queue Processes;
bool finished = false;
int WT,TA,WTA,Util,STD;

int main(int argc, char * argv[])
{

	FILE *fptr,*fptr2;
    fptr = fopen("./log.txt","w");
    fptr2 = fopen("./pref.txt","w");
    if( fptr == NULL ||fptr2 ==NULL)
    {
        printf("Error opening the file!");
        exit(1);
    }
    //char *headerLog = "#At time x process y state arr w total z remain y wait k\n";
    //fprintf(fptr,"%s",headerLog);
    
    signal(SIGUSR1, receiveProcess);
    signal(SIGUSR2, lastProcess);
    Algorithm = atoi(argv[1]);
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

    switch(Algorithm)
    {
        case 1:
            HPF();
            break;            
        case 2:
        	SRTN();
            break;
        default:
            RR(atoi(argv[2]));
            break;
    }


    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    
    fclose(fptr);
    fclose(fptr2);
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
            receivedProcess.lastTime = message.ArrivalTime;
            receivedProcess.processId = -1;
            receivedProcess.RunTime = message.RunTime;
            receivedProcess.Priority = message.Priority;
            receivedProcess.WaitingTime = 0;
            receivedProcess.RemainingTime = message.RunTime;
            receivedProcess.next = NULL;
            // printf("%d %d %d %d %d %d\n", receivedProcess.Id, receivedProcess.ArrivalTime, receivedProcess.Priority, receivedProcess.RunTime, receivedProcess.RemainingTime, receivedProcess.WaitingTime);
            push(&Processes, &receivedProcess, Algorithm);
            //printQueue(&Processes);
        }
    } while(rec_val!=-1);
}

void lastProcess(int signum){
    finished = true;
}


void HPF()
{
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
}

void SRTN()
{
    int startTime = -1, currentRemaining = -1, lastUpdate = 0;
    struct process * CurrentProcess;
    while(!finished || (Processes.head != NULL))
    {
        if(Processes.head != NULL)
        {
        	
            if((startTime == -1) || (currentRemaining > Processes.head->RemainingTime) || (!currentRemaining))
            {
                if(startTime == -1)
                {
                    CurrentProcess = pop(&Processes);
                    startTime = getClk();
                    currentRemaining = CurrentProcess->RemainingTime;
                }
                else if(!currentRemaining){
                	//fprintf(stderr,"Hello");
                    waitpid(CurrentProcess->processId, &stat_loc, 0);
                    fprintf(stderr,"Process with ID: %d finished at %d and waited For %d \n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
                    //fflush(stdout);
                    free(CurrentProcess);
                    CurrentProcess = pop(&Processes);
                    
                    CurrentProcess->WaitingTime += (getClk() - CurrentProcess->lastTime);
      
                    startTime = getClk();
                    currentRemaining = CurrentProcess->RemainingTime;
                    if(CurrentProcess->processId != -1) 
                    {
                        kill(CurrentProcess->processId, SIGCONT);
                         fprintf(stderr,"Process with id %d has continued at %d \n=========\n", CurrentProcess->Id, getClk());
                         //fflush(stdout);
                    }
                }
                else
                {
                    kill(CurrentProcess->processId, SIGSTOP);
                    CurrentProcess->RemainingTime = currentRemaining;
                    CurrentProcess->lastTime = getClk();
                  
                    push(&Processes, CurrentProcess, Algorithm);
                    fprintf(stderr,"Process with id %d has stopped at %d \n=======\n", CurrentProcess->Id, getClk());
                     //fflush(stdout);
                    // new process
                    CurrentProcess = pop(&Processes);
                    CurrentProcess->WaitingTime +=(getClk() - CurrentProcess->lastTime);
                 
                    startTime = getClk();
                    currentRemaining = CurrentProcess->RemainingTime;
                    if(CurrentProcess->processId != -1) 
                    {
                        kill(CurrentProcess->processId, SIGCONT);
                        fprintf(stderr,"Process with id %d has continued at %d \n=======\n", CurrentProcess->Id, getClk());
                         //fflush(stdout);
                    }
                }
                if(CurrentProcess->processId == -1)
                {
                    //fprintf(stderr,"HELLO\n");
                    pid = fork();
                    CurrentProcess->processId = pid;
                    if(!pid)
                    {
                        fprintf(stderr,"Process with id %d has started at %d\n", CurrentProcess->Id, getClk());
                        //fflush(stdout);
                        char number[6];
                        snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                        execl("process.out", "process.out", number, NULL);
                    }
                }
                
            }
        }
        if(getClk() - lastUpdate > 0)
        {
            //fprintf(stderr, "CR :%d  LU: %d Clk: %d\n",currentRemaining,lastUpdate,getClk());
            currentRemaining -= (getClk() - lastUpdate);
            lastUpdate = getClk();
        }
    }
    waitpid(CurrentProcess->processId, &stat_loc, 0);
     fprintf(stderr,"Process with ID: %d, finished at %d and waited for %d\n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
     //fflush(stdout);
    free(CurrentProcess);
}


void RR(int Quantum)
{
    int currentRuning = Quantum + 1, currentRemaining = 1, lastUpdate = 0, startTime = -1;
    struct process * CurrentProcess = NULL;
    while(!finished || (Processes.head != NULL))
    {
        if(Processes.head != NULL)
        {
            if(((currentRuning >= Quantum)) || (currentRemaining <= 0))
            {
                if(CurrentProcess && (currentRemaining > 0))  
                {
                    kill(CurrentProcess->processId, SIGSTOP);
                    fprintf(stderr,"Process with id %d has stopped at %d \n=======\n", CurrentProcess->Id, getClk());
                    CurrentProcess->RemainingTime = currentRemaining;
                    CurrentProcess->lastTime = getClk();
                    push(&Processes, CurrentProcess, Algorithm);
                }
                if(currentRemaining <= 0)
                {
                    waitpid(CurrentProcess->processId, &stat_loc, 0);
                    fprintf(stderr,"Process with ID: %d finished at %d and waited For %d \n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
                    free(CurrentProcess);
                }
                CurrentProcess = pop(&Processes);
                currentRemaining = CurrentProcess->RemainingTime;
                CurrentProcess->WaitingTime += (getClk() - CurrentProcess->lastTime);
                currentRuning = 0;
                if(CurrentProcess->processId == -1)
                {
                    pid = fork();
                    if(pid == 0)
                    {
                        fprintf(stderr,"Process with id %d has started at %d\n", CurrentProcess->Id, getClk());
                        char number[6];
                        snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                        execl("process.out", "process.out", number, NULL);
                    }
                    CurrentProcess->processId = pid;
                }
                else
                {
                    kill(CurrentProcess->processId, SIGCONT);
                    fprintf(stderr,"Process with id %d has continued at %d \n=======\n", CurrentProcess->Id, getClk());

                }
            }
        }
        if((getClk() - lastUpdate > 0) && (CurrentProcess != NULL))
        {
            int clk = getClk();
            if(startTime == -1)
            {
                currentRuning += (clk - CurrentProcess->ArrivalTime);
                // currentRemaining -= (clk - CurrentProcess->ArrivalTime);
                startTime = 1;
            }
            else
            {
                currentRuning += (clk - lastUpdate);
                currentRemaining -= (clk - lastUpdate);
            }
            lastUpdate = clk;
            // fprintf(stderr,"**********\nRuning Time: %d,  Remaining: %d, clock: %d\n**********\n", currentRuning, currentRemaining, clk);
            if((Processes.head == NULL) && ( currentRuning>= Quantum))
            {
                currentRuning = 0;
            }
        }
    }
    waitpid(CurrentProcess->processId, &stat_loc, 0);
    fprintf(stderr,"Process with ID: %d finished at %d and waited For %d \n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
    free(CurrentProcess);
}

