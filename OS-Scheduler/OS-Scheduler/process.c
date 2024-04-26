#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
union Semun
{
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO (Linux-specific) */
};

void down(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}
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
    usleep(100); //wait 100microseconds
    kill(getppid(),SIGCONT);
        raise(SIGSTOP);
        usleep(100); //wait 10microseconds
        kill(getppid(),SIGCONT);
    int *runPshmadd = (int *)shmat(runPshmid, 0, 0); // remainingTime

    // TODO it needs to get the remaining time from somewhere
    int currentTime = getClk();
    int remainingTime = (*runPshmadd);
    while (remainingTime > 0)
    {
        //printf("process %d remainTime=%d ", getpid(), remainingTime);
        while (currentTime == getClk()) // wait till the next clk;
            if (remainingTime == 0)
                break;
        down(sem2);
        (*runPshmadd)--;
        remainingTime = (*runPshmadd);
        up(sem1);
        printf("process%d:%d\n",getpid(), currentTime);
        // printf("%d",getpid());
        // kill(getppid(),SIGCONT);
        // raise(SIGSTOP);
        // usleep(100); //wait 10microseconds
        // kill(getppid(),SIGCONT);
        currentTime = getClk(); // update the time
    }

    destroyClk(false);

    return 0;
}
