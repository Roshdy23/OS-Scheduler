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
int forkProcess(struct Process *p)
{
    p->pid = 0;
    p->pid = fork();
    p->waitingTime = 0;
    p->remainingTime = p->runtime;

    if (p->pid == 0)
    {
        execl("./process.out", "process", NULL);
    }
    else
    {
        printf("the scheduler forked a process with id=%d at %d\n", p->id, getClk());
        return p->pid;
    }
}

void stopProcess(struct Process *p)
{
    p->lastStopTime = getClk();
    printf("the scheduler stoped process with id=%d\n", p->id);
    kill(p->pid, SIGSTOP);
    p->remainingTime = (*runPshmadd);
}

void contProcess(struct Process *p)
{
    // p->waitingTime += getClk() - p->lastStopTime;
    printf("the scheduler sent continue signal to process with id=%d\n", p->id);
    kill(p->pid, SIGCONT);
    (*runPshmadd) = p->remainingTime;
}

//------------------------ RR algorithm -----------------------------------//
void RR(int quantum)
{
    printf("executing RR algorithm\n");
    int remainQuantum = quantum;

    recievedProcesses = (struct Process *)malloc(numofProcesses * sizeof(struct Process));
    struct Process *recv;

    int currentTime;

    while (true)
    {
        // initialize the time
        currentTime = getClk();

        // if all processes are terminated
        if (terminatedProcessesNum == numofProcesses)
            break;

        // check if there is any recieved processes
        while (currentTime == getClk())
            if (recievedProcessesNum < numofProcesses)
            {
                int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);
                while (val != -1)
                {
                    recv = &recievedProcesses[recievedProcessesNum];
                    forkProcess(recv);
                    kill(recv->pid, SIGSTOP);
                    enqueueInCQ(&readyQueue, recv);
                    recievedProcessesNum++;

                    if (!RunningProcess)
                    {
                        remainQuantum = quantum;
                        RunningProcess = dequeueOFCQ(&readyQueue);
                        contProcess(RunningProcess);
                        RunningProcess->state = running;
                    }

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
                contProcess(RunningProcess);
                RunningProcess->state = running;
            }
        }
        // running process checks
        else if (RunningProcess->remainingTime == 0 || remainQuantum == 0)
        {
            // noyify process termination
            if (RunningProcess->remainingTime == 0)
            {
                printf("process with id=%d terminated with %d\n", RunningProcess->id, RunningProcess->remainingTime);
                RunningProcess->state = terminated;
                terminatedProcessesNum++;
                RunningProcess = NULL;
            }
            // switch processes
            else
            {
                RunningProcess->state = ready; // change state to ready
                enqueueInCQ(&readyQueue, RunningProcess);
                stopProcess(RunningProcess); // send stop signal to this process
                RunningProcess = NULL;
            }
            if (!isEmpty(readyQueue))
            {
                printf("Context switching\n");
                remainQuantum = quantum;
                RunningProcess = dequeueOFCQ(&readyQueue);
                RunningProcess->state = running;
                (*runPshmadd) = RunningProcess->remainingTime;
                contProcess(RunningProcess);
            }
            else
                RunningProcess = NULL;
        }
        // get remainig time & reduce the remaining quantum
        if (RunningProcess != NULL && RunningProcess->remainingTime > 0 && remainQuantum > 0)
        {
            RunningProcess->remainingTime = (*runPshmadd);
            remainQuantum -= 1;
        }

        printf("%d\n", currentTime);
    }
}

//---------------------HPF---------------------------------------------
void HPF()
{

}
//---------------------------------------------------------------//