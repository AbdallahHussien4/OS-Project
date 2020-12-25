#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void writeLogs(FILE *fptr,int time,int process,char* state,int w,int z,int y,int k){
    if( y == 0){
        float TA = (w == 1)?time:time - w ;
        float WTA = TA/(z) ;
        fprintf(fptr,"AT time %d process %d %s arr %d total %d remain %d wait %d  TA %.2f WTA %.2f \n",time,process,state,w,z,y,k,TA,WTA);
    }
    else {
        fprintf(fptr,"AT time %d process %d %s arr %d total %d remain %d wait %d \n",time,process,state,w,z,y,k);
    }
}
void writeStatus(FILE *fptr,float util, float avgWTA , float avgW , float stdWTA){
    fprintf(fptr,"CPU utilization = %.2f%% \n",util);
    fprintf(fptr,"Avg WTA = %.2f \n",avgWTA);
    fprintf(fptr,"Avg Waiting = %.2f \n",avgW);
    fprintf(fptr,"Std WTA = %.2f \n",stdWTA);
}

int main(){
    FILE *fptr;
    // Open the file to wite in it
    fptr = fopen("./output.txt","w");
    if( fptr == NULL){
        printf("Error opening the file!");
        exit(1);
    }
    // Write the file header in the output schedular.log
    char *headerLog = "#At time x process y state arr w total z remain y wait k\n";
    fprintf(fptr,"%s",headerLog);
    // Test case for the output => schedular.log
    writeLogs(fptr,1,1,"started",1,6,6,0);
    writeLogs(fptr,3,1,"stoped",1,6,4,0);
    writeLogs(fptr,3,2,"started",3,3,3,0);
    writeLogs(fptr,6,2,"finished",3,3,0,0);
    writeLogs(fptr,6,1,"resumed",1,6,4,3);
    writeLogs(fptr,10,1,"finished",1,6,0,3);
    fclose(fptr);

    FILE *fptr2;
    fptr2 = fopen("./output2.txt","w");
    writeStatus(fptr2,100,1.34,1.5,0.34);
    fclose(fptr2);

    return 0;
}