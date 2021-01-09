#include "headers.h"


void clearResources(int);

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    
    // TODO Initialization

    // 1. Read the input files.
    struct process arr[100000];
    FILE * file_pointer;
    file_pointer = fopen("processes.txt", "r");
    char str1[3], str2[7], str3[7], str4[8],  str5[8];
    fscanf(file_pointer, "%s %s %s %s %s", str1, str2, str3, str4, str5);
    int p_num=0;
    while(1){
        long id,arrival,runt,p, memory;
        if(fscanf(file_pointer, "%ld %ld %ld %ld %ld", &id, &arrival, &runt, &p, &memory)!=EOF)
        {
            arr[p_num].ArrivalTime = arrival;
            arr[p_num].RunTime = runt;
            arr[p_num].Priority = p;
            arr[p_num].Id = p_num+1;
            arr[p_num].memory = memory;
            p_num++;
        }
        else break;
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    char ch, Q[2];
    printf("Choose scheduler algorithm \n");
    printf("1 for HPF \n");
    printf("2 for SRTN \n");
    printf("3 for RR \n");
    scanf("%c", &ch);
    
    if(ch=='3')
    {
        printf("Please enter your Quantum : \n");
        scanf("%s", Q);
    }
    // 3. Initiate and create the scheduler and clock processes.
    int pid;
    for(int i = 0; i < 2; i++)
    {
        pid = fork();
        if (pid == -1)
      	    perror("error in fork1");
  	
        else if (!pid)
        {
            if(!i)
                execl("clk.out", "clk.out", NULL);
            else
                execl("scheduler.out", "scheduler.out", &ch, &Q, NULL);    
        }
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.

    key_t key_id;
    int msgq_id, send_val;

    key_id = ftok("gen_schdlr_com", 65);
    msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    //printf("Message Queue ID = %d\n", msgq_id);

    struct msgbuffer message;

    int Iterator=0;
    long CurrentClk=-1;
    bool flag;
    while(Iterator<p_num)
    {
        flag = false;
        // To get time use this
        if(CurrentClk != getClk())
        {
            CurrentClk = getClk();
            for(int i = Iterator; i <= p_num; i++)
            {
                if(arr[i].ArrivalTime==CurrentClk)
                {
                    message.mtype = 7; 
                    message.Id = arr[i].Id;
                    message.ArrivalTime = arr[i].ArrivalTime;
                    message.RunTime = arr[i].RunTime;
                    message.Priority = arr[i].Priority;
                    message.memory = arr[i].memory;
                    send_val = msgsnd(msgq_id, &message, sizeof(message), !IPC_NOWAIT);
                    if (send_val == -1)
                        perror("Error in send");
                    flag = true;
                }
                else
                {
                    Iterator=i;
                    break;
                }
            }
            if(flag)
                {
                	kill(pid, SIGUSR1);
        	 }
        }
    }
    sleep(2);
    kill(pid, SIGUSR2);
    int stat_loc;
    waitpid(pid, &stat_loc, 0);
    // 7. Clear clock resourced
    destroyClk(true);
}

void clearResources(int signum)
{
    //msgctl(signum, IPC_RMID, (struct msqid_ds *)0);
    //TODO Clears all resources in case of interruption
}
