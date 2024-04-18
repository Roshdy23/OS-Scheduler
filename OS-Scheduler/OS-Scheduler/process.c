#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char *argv[])
{
    initClk();
    key_t key = ftok("key", 'r'); // shared memory to get the remain time of the running process
    int runPshmid = shmget(key, 4, 0666 | IPC_CREAT);

    if (runPshmid == -1)
    {
        perror("Error in creating message queue");
        exit(-1);
    }

    int *runPshmadd = (int *)shmat(runPshmid, 0, 0); // remainingTime
(*runPshmadd)=1;
    raise(SIGSTOP);
   
    // TODO it needs to get the remaining time from somewhere
    int currentTime = getClk();
    int remainingTime = (*runPshmadd);

    while (remainingTime > 0)
    {
       // printf("process %d remainTime=%d\n", getpid(), remainingTime);
        (*runPshmadd)--;
        remainingTime = (*runPshmadd);

        while (currentTime == getClk()) // wait till the next clk;
            if (remainingTime == 0)
                break;

        currentTime = getClk(); // update the time
    }

    destroyClk(false);

    printf("end\n");

    return 0;
}
