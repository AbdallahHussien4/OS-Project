#include "headers.h"

void receiveProcess(int signum);
void lastProcess(int signum);
void HPF(FILE *fptr, FILE * m);
void SRTN(FILE *fptr, FILE * m);
void RR(int Quantum,FILE *fptr, FILE * m);
void writeStatus(FILE *fptr,float util, float avgWTA , float avgW);
void writeLogs_scheduler(FILE *fptr,int time,int process,char* state,int w,int z,int y,int k);
void writeLogs_memory(FILE *fptr, int time, int bytes, int process, int start, int end, bool flag);
int Algorithm, rec_val, stat_loc, msgq_id, pid,LastFinish=0;
Queue Processes, ReadyQueue;
Memory * memory;	
Array Wt;
bool finished = false;
float TWT=0,TTA=0,TWTA=0,Wasted=0,ProcessesNum=0;
double STD=0;

int main(int argc, char * argv[])
{
	FILE *fptr, *fptr2, *m;
    fptr = fopen("./scheduler.log","w");
    m = fopen("./memory.log","w");
    fptr2 = fopen("./scheduler.perf","w");
    if( fptr == NULL || fptr2 == NULL || m == NULL)
    {
        printf("Error opening the file!");
        exit(1);
    }
    memory = initMemory();
    signal(SIGUSR1, receiveProcess);
    signal(SIGUSR2, lastProcess);
    Algorithm = atoi(argv[1]);
    printf("Algorithm Number is %d\n",Algorithm);
    initClk();
    initArray(&Wt,5);
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
            HPF(fptr,m);
            break;            
        case 2:
        	SRTN(fptr,m);
            break;
        default:
            RR(atoi(argv[2]),fptr,m);
            break;
    }


    writeStatus(fptr2,(LastFinish-Wasted)*100/LastFinish,TWTA/ProcessesNum , TWT/ProcessesNum);
    freeArray(&Wt);
    fclose(fptr);
    fclose(fptr2);
    destroyClk(false);
}

// ===========================================================
// =============== Signal To Receive Processes ===============
// ===========================================================
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
            receivedProcess.memory = message.memory;
            receivedProcess.next = NULL;
            receivedProcess.Sector = NULL;
            push(&Processes, &receivedProcess, Algorithm);
        }
    } while(rec_val!=-1);
}

// ========================================================================================
// =============== Signal To Know if Process generator finishs its prcesses ===============
// ========================================================================================
void lastProcess(int signum){
    finished = true;
}

// =============================================
// =============== HPF Algorithm ===============
// =============================================

void HPF(FILE *fptr, FILE * m)
{

	while(!finished || (Processes.head != NULL))
    {
        if(Processes.head != NULL)
        {
            struct process * CurrentProcess = pop(&Processes);
            CurrentProcess->WaitingTime = getClk() - CurrentProcess->ArrivalTime;
            CurrentProcess->Sector = allocate(memory, CurrentProcess->memory);
            pid = fork();
            
            if(!pid)
            {
                char number[6];
                snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                execl("process.out", "process.out", number, NULL);
            }
            if(LastFinish!=0)
            	Wasted+=getClk()-LastFinish;
            writeLogs_memory(m, getClk(), CurrentProcess->memory, CurrentProcess->Id, CurrentProcess->Sector->s, CurrentProcess->Sector->e, true);
            writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"started",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
            waitpid(pid, &stat_loc, 0);
            LastFinish=stat_loc>>8;
            writeLogs_memory(m, LastFinish, CurrentProcess->memory, CurrentProcess->Id, CurrentProcess->Sector->s, CurrentProcess->Sector->e, false);
            writeLogs_scheduler(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
            deallocate(memory, CurrentProcess->Sector);
            free(CurrentProcess);

        }
    }
}

// ==============================================
// =============== SRTN Algorithm ===============
// ==============================================

void SRTN(FILE *fptr, FILE * m)
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
                    waitpid(CurrentProcess->processId, &stat_loc, 0);
                    LastFinish=stat_loc>>8;
                    writeLogs_memory(m, LastFinish, CurrentProcess->memory, CurrentProcess->Id, CurrentProcess->Sector->s, CurrentProcess->Sector->e, false);
                    int size = deallocate(memory, CurrentProcess->Sector);
                    writeLogs_scheduler(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
                    free(CurrentProcess);
                    if(ReadyQueue.head == NULL)
                        CurrentProcess = pop(&Processes);
                    else
                    {
                        CurrentProcess = popReady(&Processes, size);
                        if(CurrentProcess == NULL)
                            CurrentProcess = pop(&Processes);
                    }
                    
                    CurrentProcess->WaitingTime += (getClk() - CurrentProcess->lastTime);
      
                    startTime = getClk();
                    currentRemaining = CurrentProcess->RemainingTime;
                    if(CurrentProcess->processId != -1) 
                    {
                        kill(CurrentProcess->processId, SIGCONT);
                        Wasted+=getClk()-LastFinish;
                        writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"resumed",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    }
                }
                else
                {
                    kill(CurrentProcess->processId, SIGSTOP);
                    LastFinish=getClk();
                    CurrentProcess->RemainingTime = currentRemaining;
                    CurrentProcess->lastTime = getClk();
                  
                    push(&Processes, CurrentProcess, Algorithm);
                    writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"stopped",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    CurrentProcess = pop(&Processes);
                    CurrentProcess->WaitingTime +=(getClk() - CurrentProcess->lastTime);
                 
                    startTime = getClk();
                    currentRemaining = CurrentProcess->RemainingTime;
                    if(CurrentProcess->processId != -1) 
                    {
                        kill(CurrentProcess->processId, SIGCONT);
                        Wasted+=getClk()-LastFinish;
                        writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"resumed",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    }
                }
                if(CurrentProcess->processId == -1)
                {
                    CurrentProcess->Sector = allocate(memory,CurrentProcess->memory);
                    if(CurrentProcess->Sector)
                    {   
                        pid = fork();
                        CurrentProcess->processId = pid;
                        if(LastFinish!=0)
                            Wasted+=getClk()-LastFinish;
                        writeLogs_memory(m, getClk(), CurrentProcess->memory, CurrentProcess->Id, CurrentProcess->Sector->s, CurrentProcess->Sector->e, true);
                        writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"started",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                        if(!pid)
                        {
                            char number[6];
                            snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                            execl("process.out", "process.out", number, NULL);
                        }
                    }
                    else
                    {
                        push(&ReadyQueue, CurrentProcess, 4);
                    }
                }
                
            }
        }
        if(getClk() - lastUpdate > 0)
        {
            currentRemaining -= (getClk() - lastUpdate);
            lastUpdate = getClk();
        }
    }
    waitpid(CurrentProcess->processId, &stat_loc, 0);
    LastFinish=stat_loc>>8;
    writeLogs_memory(m, LastFinish, CurrentProcess->memory, CurrentProcess->Id, CurrentProcess->Sector->s, CurrentProcess->Sector->e, false);
    writeLogs_scheduler(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
    free(CurrentProcess);
}


// ============================================
// =============== RR Algorithm ===============
// ============================================

void RR(int Quantum,FILE *fptr, FILE * m)
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
                    CurrentProcess->RemainingTime = currentRemaining;
                    CurrentProcess->lastTime = getClk();
                    writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"stopped",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    push(&Processes, CurrentProcess, Algorithm);
                }
                if(currentRemaining <= 0)
                {
                    waitpid(CurrentProcess->processId, &stat_loc, 0);
                    LastFinish=stat_loc>>8;
                    writeLogs_scheduler(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
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
                        char number[6];
                        snprintf(number, sizeof(number), "%d", CurrentProcess->RemainingTime);
                        execl("process.out", "process.out", number, NULL);
                    }
                    if(LastFinish!=0)
                    	Wasted+=getClk()-LastFinish;
                    writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"started",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);
                    CurrentProcess->processId = pid;
                }
                else
                {
                    kill(CurrentProcess->processId, SIGCONT);
                    Wasted+=getClk()-LastFinish;
                    writeLogs_scheduler(fptr,getClk(),CurrentProcess->Id,"resumed",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,CurrentProcess->RemainingTime,CurrentProcess->WaitingTime);

                }
            }
        }
        if((getClk() - lastUpdate > 0) && (CurrentProcess != NULL))
        {
            int clk = getClk();
            if(startTime == -1)
            {
                currentRuning += (clk - CurrentProcess->ArrivalTime);
                startTime = 1;
            }
            else
            {
                currentRuning += (clk - lastUpdate);
                currentRemaining -= (clk - lastUpdate);
            }
            lastUpdate = clk;
            if((Processes.head == NULL) && ( currentRuning>= Quantum))
            {
                currentRuning = 0;
            }
        }
    }
    waitpid(CurrentProcess->processId, &stat_loc, 0);
    LastFinish=stat_loc>>8;
    writeLogs_scheduler(fptr,stat_loc>>8,CurrentProcess->Id,"finished",CurrentProcess->ArrivalTime,CurrentProcess->RunTime,0,CurrentProcess->WaitingTime);
    free(CurrentProcess);
}

// ==================================================
// =============== Printing Functions ===============
// ==================================================

void writeLogs_scheduler(FILE *fptr,int time,int process,char* state,int w,int z,int y,int k){
    if( state == "finished"){
        float TA = (w == 1)?time:time - w ;
        float WTA = TA/(z) ;
        TTA+=TA;
        TWTA+=WTA;
        TWT+=k;
        insertArray(&Wt,WTA);
        ProcessesNum+=1;
        fprintf(fptr,"At time %d process %d %s arr %d total %d remain %d wait %d  TA %.2f WTA %.2f \n",time,process,state,w,z,y,k,TA,WTA);
    }
    else {
        fprintf(fptr,"At time %d process %d %s arr %d total %d remain %d wait %d \n",time,process,state,w,z,y,k);
    }
}

void writeLogs_memory(FILE *fptr, int time, int bytes, int process, int start, int end, bool flag)
{
    if(flag)
    {
        fprintf(fptr,"At time %d allocated %d bytes for process %d from %d to %d\n", time, bytes, process, start, end);
    }
    else {
        fprintf(fptr,"At time %d freed %d bytes from process %d from %d to %d\n", time, bytes, process, start, end);
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


