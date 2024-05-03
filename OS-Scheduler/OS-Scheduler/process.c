#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char *argv[])
{
    int sem1 = semget('5', 1, 0666 | IPC_CREAT);
    int sem2 = semget('6', 1, 0666 | IPC_CREAT);

    if (sem1 == -1 || sem2 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }

    initClk();

    key_t key = ftok("key", 'r'); // shared memory to get the remain time of the running process
    int runPshmid = shmget(key, 4, 0666 | IPC_CREAT);

    if (runPshmid == -1)
    {
        perror("Error in creating message queue");
        exit(-1);
    }
    int *runPshmadd = (int *)shmat(runPshmid, 0, 0); // remainingTime
    
    // to handle synchronization between scheduler
    usleep(100); //wait 100microseconds
    kill(getppid(),SIGCONT);
    raise(SIGSTOP);
    usleep(100); //wait 10microseconds
    kill(getppid(),SIGCONT);

    // run the process
    int currentTime = getClk();
    int remainingTime = (*runPshmadd);
    while (remainingTime > 0)
    {
        while (currentTime == getClk()) // wait till the next clk;
        if (remainingTime == 0)
            break;
        down(sem2); // wait for scheduler
        (*runPshmadd)--;
        remainingTime = (*runPshmadd);
        up(sem1); // up the scheduler
        currentTime = getClk(); // update the time
    }

    destroyClk(false);

    return 0;
}
