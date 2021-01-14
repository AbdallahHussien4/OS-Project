#include "headers.h"


int main(){
    struct process arr;
    Queue Processes;
    FILE * file_pointer;
    file_pointer = fopen("processes.txt", "r");
    char str1[3], str2[7], str3[7], str4[8],  str5[8];
    fscanf(file_pointer, "%s %s %s %s %s", str1, str2, str3, str4, str5);
    int p_num=0;
    while(1){
        long id,arrival,runt,p, memory;
        if(fscanf(file_pointer, "%ld %ld %ld %ld %ld", &id, &arrival, &runt, &p, &memory)!=EOF)
        {
            arr.ArrivalTime = arrival;
            arr.RunTime = runt;
            arr.Priority = p;
            arr.Id = p_num+1;
            arr.memory = memory;        
            arr.WaitingTime = 0;
            arr.RemainingTime = 0;
            arr.lastTime = 0;
            arr.processId = -1;
            arr.next = NULL;
            arr.Sector = NULL;
            p_num++;
            push(&Processes, &arr, 4);
            fprintf(stderr, "3adeit mnhom\n");
        }
        else break;
    }
    printQueue(&Processes);
    
    if(popReady(&Processes, 400))
        fprintf(stderr, "bazit\n");
    else
        fprintf(stderr, "sh3'ala\n");    
    popReady(&Processes, 230);
    printQueue(&Processes);
    popReady(&Processes, 186);
    printQueue(&Processes);
    popReady(&Processes, 203);
    printQueue(&Processes);
    popReady(&Processes, 120);
    printQueue(&Processes);
    popReady(&Processes, 186);
    printQueue(&Processes);

    return 0;
}

