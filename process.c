#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    // while (remainingtime > 0)
    // {
    //     // remainingtime = ??;
    // }
    printf("ana f el process");
    printf("%d", atoi(argv[1]));
    sleep(2);

    // destroyClk(false);
    
    return 0;
}
