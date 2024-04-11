#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char *argv[])
{
    initClk();

    key_t key = ftok("key", 'r'); // shared memory to get the remain time of the running process
    int runPshmid = shmget(key, 4, 0666 | IPC_CREAT);

    int *runPshmadd = (int *)shmat(runPshmid, (void *)0, 0); // remainingTime

    // TODO it needs to get the remaining time from somewhere
    int currentTime = getClk();

    while ((*runPshmadd) > 0)
    {
        (*runPshmadd)--;
        while (currentTime == getClk()) // wait till the next clk;
            ;
        currentTime = getClk(); // update the time
    }

    destroyClk(false);

    //printf("process with PID=%d terminated\n", getpid());

    exit(0);
    return 0;
}
