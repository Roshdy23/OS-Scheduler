#include "headers.h"

void RR(int quantum);
void HPF();
int quantum;
struct CircularQueue readyQueue; // queue to store arrived processes

int numofProcesses;             // total number of processes comming from generator
int recievedProcessesNum = 0;   // number of arrived processes
int terminatedProcessesNum = 0; // total number of terminated processes
int algorithm = -1;
struct Process *RunningProcess = NULL;
struct Process *recievedProcesses;

int msgqid, sigshmid;
int *sigshmaddr;

int runPshmid; // shared memory for the remain time of the running process
int *runPshmadd;

int idealTime = 0;
int prevTime;

//-------------------------------main function---------------------------//
int main(int argc, char *argv[])
{
    initClk();
    while (getClk() == 0)
        ;
    printf("hi from scheduler\n");

    initializeCircularQueue(&readyQueue);
    key_t key = ftok("key", 'p');
    msgqid = msgget(key, 0666 | IPC_CREAT); // this message queue will comunicate process generator with scheduler

    if (msgqid == -1)
    {
        perror("Error in creating message queue");
        exit(-1);
    }

    algorithm = atoi(argv[1]);
    quantum = atoi(argv[2]);
    numofProcesses = atoi(argv[3]);

    //    TODO implement the scheduler :)

    key_t shmkey = ftok("key", 'r'); // shared memory for the remain time of the running process
    runPshmid = shmget(shmkey, 4, 0666 | IPC_CREAT);
    runPshmadd = (int *)shmat(runPshmid, (void *)0, 0);

    raise(SIGSTOP);

    switch (algorithm)
    {
    case 1:
        HPF();
        break;
    case 2:
        RR(quantum);
        break;

    default:
        break;
    }

    // upon termination release the clock resources.
    // destroyClk(true);
}

//-----------------------------processes operation functions--------------------//
void forkProcess(struct Process *p)
{
    p->pid = 0;
    p->pid = fork();
    p->waitingTime = 0;
    p->remainingTime = p->runtime;
    p->lastStopTime = prevTime;
    (*runPshmadd) = 0;
    if (p->pid == 0)
    {
        printf("forked\n");
        execl("./process.out", "./process.out", NULL);
    }
    else
    {
        printf("the scheduler forked a process with id=%d at %d\n", p->id, getClk());
        while ((*runPshmadd) == 0)
            ;
    }
}

void stopProcess(struct Process *p)
{
    kill(p->pid, SIGSTOP);
    p->lastStopTime = prevTime;
}

void contProcess(struct Process *p)
{
    p->waitingTime += prevTime - p->lastStopTime;
    (*runPshmadd) = p->remainingTime;
    kill(p->pid, SIGCONT);
}

//------------------------ RR algorithm -----------------------------------//
void RR(int quantum)
{
    printf("executing RR algorithm\n");
    int remainQuantum = quantum;

    recievedProcesses = (struct Process *)malloc(numofProcesses * sizeof(struct Process));
    struct Process *recv;

    prevTime = getClk();

    while (true)
    {
        // if all processes are terminated
        if (terminatedProcessesNum == numofProcesses)
            break;

        // initialize the time
        prevTime = getClk();

        printf("clk:%d\n", getClk());

        if (RunningProcess && RunningProcess->remainingTime == 0)
        {
            printf("%d\n", RunningProcess->remainingTime);
            if (RunningProcess->remainingTime == 0)
            {
                wait(NULL);
                printf("At time %d process %d terminated arr %d total %d remain %d wait %d\n", getClk(), RunningProcess->id, RunningProcess->arrival_time, RunningProcess->runtime, RunningProcess->remainingTime, RunningProcess->waitingTime);
                RunningProcess->state = terminated;
                terminatedProcessesNum++;
                RunningProcess = NULL;
            }
        }

        if (recievedProcessesNum < numofProcesses)
        {
            int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);
            // printf("val %d\n",val);
            while (val != -1)
            {
                recv = &recievedProcesses[recievedProcessesNum];
                forkProcess(recv);
                enqueueInCQ(&readyQueue, recv);
                recievedProcessesNum++;

                val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);
            }
        }

        // running new process
        if (RunningProcess == NULL)
        {
            if (!isEmpty(readyQueue))
            {
                remainQuantum = quantum;
                RunningProcess = dequeueOFCQ(&readyQueue);
                printf("At time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), RunningProcess->id, RunningProcess->arrival_time, RunningProcess->runtime, RunningProcess->remainingTime, RunningProcess->waitingTime);
                contProcess(RunningProcess);
                RunningProcess->state = running;
            }
        }

        // running process checks
        else if (remainQuantum == 0)
        {
            RunningProcess->state = ready; // change state to ready
            enqueueInCQ(&readyQueue, RunningProcess);
            stopProcess(RunningProcess); // send stop signal to this process
            printf("At time %d process %d stopped arr %d total %d remain %d wait %d\n", getClk(), RunningProcess->id, RunningProcess->arrival_time, RunningProcess->runtime, RunningProcess->remainingTime, RunningProcess->waitingTime);
            RunningProcess = NULL;
            if (!isEmpty(readyQueue))
            {
                printf("Context switching\n");
                remainQuantum = quantum;
                RunningProcess = dequeueOFCQ(&readyQueue);
                RunningProcess->state = running;
                printf("At time %d process %d started arr %d total %d remain %d wait %d\n", getClk(), RunningProcess->id, RunningProcess->arrival_time, RunningProcess->runtime, RunningProcess->remainingTime, RunningProcess->waitingTime);
                contProcess(RunningProcess);
            }
            else
                RunningProcess = NULL;
        }

        // get remainig time & reduce the remaining quantum
        if (RunningProcess)
        {
            RunningProcess->remainingTime--;
            remainQuantum -= 1;
        }

        // wait till the next step
        while (prevTime == getClk())
            ;
    }

    printf("the ideal time is %d\n", idealTime);
    // total time
}

//---------------------HPF---------------------------------------------
void HPF()
{
}
//---------------------------------------------------------------//