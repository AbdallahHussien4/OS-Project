#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    remainingtime = atoi(argv[1]);
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    int CurrentClk = getClk();
    while (remainingtime > 0)
    {
        if(CurrentClk != getClk())
        {
        	//fprintf(stderr,"Process Gowa\n ");
            remainingtime -= (getClk() - CurrentClk) ;
            CurrentClk = getClk();
        }
    }
    // printf("%d", atoi(argv[1]));
    int clk = getClk();
    fprintf(stderr,"\n\n remaining %d , clk %d\n\n", remainingtime, clk);
    destroyClk(false);
    exit(clk);
}
