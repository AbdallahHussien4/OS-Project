#include "headers.h"

void receiveProcess(int signum);
void lastProcess(int signum);
void HPF(FILE *fptr);
void SRTN(FILE *fptr);
void RR(int Quantum,FILE *fptr);
void writeStatus(FILE *fptr,float util, float avgWTA , float avgW);
void writeLogs(FILE *fptr,int time,int process,char* state,int w,int z,int y,int k);

int Algorithm, rec_val, stat_loc, msgq_id, pid,LastFinish=0, msgq;
Queue Processes;	
Array Wt;
bool finished = false;
float TWT=0,TTA=0,TWTA=0,Wasted=0,ProcessesNum=0;
double STD=0;
struct remain message;
int main(int argc, char * argv[])
{
	
	FILE *fptr,*fptr2;
    fptr = fopen("./scheduler.log","w");
    fptr2 = fopen("./scheduler.perf","w");
    if( fptr == NULL ||fptr2 ==NULL)
    {
        printf("Error opening the file!");
        exit(1);
    }
    
    signal(SIGUSR1, receiveProcess);
    signal(SIGUSR2, lastProcess);
    Algorithm = atoi(argv[1]);
    printf("Algorithm Number is %d\n",Algorithm);
    initClk();
    initArray(&Wt,5);
    key_t key_id;
    

    key_id = ftok("gen_schdlr_com", 65);               
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); 
    msgq = msgget(60, 0666 | IPC_CREAT); 
    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    switch(Algorithm)
    {
        case 1:
            HPF(fptr);
            break;            
        case 2:
        	SRTN(fptr);
            break;
        default:
            RR(atoi(argv[2]),fptr);
            break;
    }


    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    writeStatus(fptr2,(LastFinish-Wasted)*100/LastFinish,TWTA/ProcessesNum , TWT/ProcessesNum);
    freeArray(&Wt);
    fclose(fptr);
    fclose(fptr2);
    destroyClk(false);
}

// =====================================================================
// =============== Signal To Receive Processes =========================
// =====================================================================
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

// ==================================================================================================
// =============== Signal To Know if Process generator finishs its prcesses =========================
// ==================================================================================================
void lastProcess(int signum){
    finished = true;
}

// =====================================================================
// =============== HPF Algorithm ========================================
// =====================================================================

void HPF(FILE *fptr)
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
                // fprintf(stderr,"Process with id %d has started at %d\n", CurrentProcess->Id, getClk());
                char number[6];
                snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                execl("process.out", "process.out", number, NULL);
            }
            // if(LastFinish!=0)
            Wasted+=getClk()-LastFinish;
            writeLogs(fptr,getClk(),CurrentProcess->Id,"started",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
            int last = getClk();
            while(CurrentProcess->RemainingTime > 0)
            {   
                if(getClk() - last >0)
                {
                    last = getClk();
                    message.remainig = --(CurrentProcess->RemainingTime);
                    msgsnd(msgq, &message, sizeof(message.remainig), !IPC_NOWAIT);
                }
            }
            waitpid(pid, &stat_loc, 0);
            LastFinish=stat_loc>>8;
            writeLogs(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
            free(CurrentProcess);

        }
    }
}

// =====================================================================
// =============== SRTN Algorithm ========================================
// =====================================================================

void SRTN(FILE *fptr)
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
                    lastUpdate = getClk();
                    currentRemaining = CurrentProcess->RemainingTime;
                }
                else if(!currentRemaining){
                	//// fprintf(stderr,"Hello");
                    waitpid(CurrentProcess->processId, &stat_loc, 0);
                    LastFinish=stat_loc>>8;
                    writeLogs(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
                    // fprintf(stderr,"Process with ID: %d finished at %d and waited For %d \n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
                    //fflush(stdout);
                    free(CurrentProcess);
                    CurrentProcess = pop(&Processes);
                    
                    CurrentProcess->WaitingTime += (getClk() - CurrentProcess->lastTime);
      
                }
                else
                {
                    kill(CurrentProcess->processId, SIGSTOP);
                    LastFinish=getClk();
                    CurrentProcess->RemainingTime = currentRemaining;
                    CurrentProcess->lastTime = getClk();
                  
                    push(&Processes, CurrentProcess, Algorithm);
                    writeLogs(fptr,getClk(),CurrentProcess->Id,"stopped",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    // fprintf(stderr,"Process with id %d has stopped at %d \n=======\n", CurrentProcess->Id, getClk());
                    //fflush(stdout);
                    // new process
                    CurrentProcess = pop(&Processes);
                    CurrentProcess->WaitingTime +=(getClk() - CurrentProcess->lastTime);
                }
                if(CurrentProcess->processId == -1)
                {
                    //// fprintf(stderr,"HELLO\n");
                    pid = fork();
                    CurrentProcess->processId = pid;
                    currentRemaining = CurrentProcess->RemainingTime;
                    // if(LastFinish!=0)
                    Wasted+=getClk()-LastFinish;
                    writeLogs(fptr,getClk(),CurrentProcess->Id,"started",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    if(!pid)
                    {
                        // fprintf(stderr,"Process with id %d has started at %d\n", CurrentProcess->Id, getClk());
                        //fflush(stdout);
                        char number[6];
                        snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                        execl("process.out", "process.out", number, NULL);
                    }
                    fprintf(stderr, "process %d with pid %d has started \n", CurrentProcess->Id, CurrentProcess->processId);
                }else
                {
                    startTime = getClk();
                    currentRemaining = CurrentProcess->RemainingTime;
                    
                    kill(CurrentProcess->processId, SIGCONT);
                    Wasted+=getClk()-LastFinish;
                    writeLogs(fptr,getClk(),CurrentProcess->Id,"resumed",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    // fprintf(stderr,"Process with id %d has resumed at %d \n=======\n", CurrentProcess->Id, getClk());
                        //fflush(stdout);
                    
                }
                
            }
        }
        if(CurrentProcess && (getClk() - lastUpdate > 0))
        {
            // fprintf(stderr, "CR :%d  LU: %d Clk: %d\n",currentRemaining,lastUpdate,getClk());
            currentRemaining -= (getClk() - lastUpdate);
            lastUpdate = getClk();
            message.remainig = currentRemaining;
            msgsnd(msgq, &message, sizeof(message.remainig), !IPC_NOWAIT);
        }
    }
    while(currentRemaining || (getClk() - lastUpdate > 0))
    {
        if(getClk() - lastUpdate > 0)
        {
            currentRemaining -= (getClk() - lastUpdate);
            lastUpdate = getClk();
            message.remainig = currentRemaining;
            msgsnd(msgq, &message, sizeof(message.remainig), !IPC_NOWAIT);
        }
    }
    waitpid(CurrentProcess->processId, &stat_loc, 0);
    LastFinish=stat_loc>>8;
    writeLogs(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
     // fprintf(stderr,"Process with ID: %d, finished at %d and waited for %d\n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
     //fflush(stdout);
    free(CurrentProcess);
}


// =====================================================================
// =============== RR Algorithm ========================================
// =====================================================================

void RR(int Quantum,FILE *fptr)
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
                    LastFinish=getClk();
                    // fprintf(stderr,"Process with id %d has stopped at %d \n=======\n", CurrentProcess->Id, getClk());
                    CurrentProcess->RemainingTime = currentRemaining;
                    CurrentProcess->lastTime = getClk();
                    writeLogs(fptr,getClk(),CurrentProcess->Id,"stopped",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    push(&Processes, CurrentProcess, Algorithm);
                }
                if(currentRemaining <= 0)
                {
                    waitpid(CurrentProcess->processId, &stat_loc, 0);
                    LastFinish=stat_loc>>8;
                    writeLogs(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
                    // fprintf(stderr,"Process with ID: %d finished at %d and waited For %d \n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
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
                        // fprintf(stderr,"Process with id %d has started at %d\n", CurrentProcess->Id, getClk());
                        char number[6];
                        snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                        execl("process.out", "process.out", number, NULL);
                    }
                    // if(LastFinish!=0)
                    Wasted+=getClk()-LastFinish;
                    writeLogs(fptr,getClk(),CurrentProcess->Id,"started",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    CurrentProcess->processId = pid;
                }
                else
                {
                    kill(CurrentProcess->processId, SIGCONT);
                    Wasted+=getClk()-LastFinish;
                    writeLogs(fptr,getClk(),CurrentProcess->Id,"resumed",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    // fprintf(stderr,"Process with id %d has resumed at %d \n=======\n", CurrentProcess->Id, getClk());

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
            // // fprintf(stderr,"**********\nRuning Time: %d,  Remaining: %d, clock: %d\n**********\n", currentRuning, currentRemaining, clk);
            if((Processes.head == NULL) && ( currentRuning>= Quantum))
            {
                currentRuning = 0;
            }
            message.remainig = currentRemaining;
            msgsnd(msgq, &message, sizeof(message.remainig), !IPC_NOWAIT);
        }
    }
    while(currentRemaining || (getClk() - lastUpdate > 0))
    {
        if(getClk() - lastUpdate > 0)
        {
            currentRemaining -= (getClk() - lastUpdate);
            lastUpdate = getClk();
            message.remainig = currentRemaining;
            msgsnd(msgq, &message, sizeof(message.remainig), !IPC_NOWAIT);
        }
    }
    waitpid(CurrentProcess->processId, &stat_loc, 0);
    LastFinish=stat_loc>>8;
    writeLogs(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
    // fprintf(stderr,"Process with ID: %d finished at %d and waited For %d \n", CurrentProcess->Id, getClk(),CurrentProcess->WaitingTime);
    free(CurrentProcess);
}

// ===================================================
// =============== Printing Functions ================
// ===================================================

void writeLogs(FILE *fptr,int time,int process,char* state,int w,int z,int y,int k){
    if(state == "finished")
    {
        float TA = (w == 1)?time:time - w ;
        float WTA;
        if(z)
            WTA = TA/(z);
        else
            WTA = TA;
        TTA+=TA;
        TWTA+=WTA;
        TWT+=k;
        insertArray(&Wt,WTA);
        ProcessesNum+=1;
        fprintf(fptr,"AT time %d process %d %s arr %d total %d remain %d wait %d  TA %.2f WTA %.2f \n",time,process,state,w,z,y,k,TA,WTA);
    }
    else {
        fprintf(fptr,"AT time %d process %d %s arr %d total %d remain %d wait %d \n",time,process,state,w,z,y,k);
    }
}
void writeStatus(FILE *fptr,float util, float avgWTA , float avgW){

	//Calculate STD
	double totalSum = 0;
    for(int i =0; i<ProcessesNum;i++){
        Wt.array[i] -= avgWTA;
        totalSum += Wt.array[i]*Wt.array[i];
    }
    STD = totalSum/ProcessesNum;
    STD = sqrt(STD);
    
    fprintf(fptr,"CPU utilization = %.2f%% \n",util);
    fprintf(fptr,"Avg WTA = %.2f \n",avgWTA);
    fprintf(fptr,"Avg Waiting = %.2f \n",avgW);
    fprintf(fptr,"Std WTA = %.2f \n",STD);
}


