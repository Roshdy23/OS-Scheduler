#include "headers.h"


void RR(int quantum);
void HPF();
int quantum;

int numofProcesses;             // total number of processes comming from generator
int arrivedProcessesNum = 0;    // number of arrived processes
int terminatedProcessesNum = 0; // total number of terminated processes
int algorithm = -1;
struct Process *RunningProcess;
struct Process *processRecv;

int msgqid, sigshmid;
int *sigshmaddr;

int runPshmid; // shared memory for the remain time of the running process
int *runPshmadd;
//-------------------------------main function---------------------------//
int main(int argc, char *argv[])
{
    initClk();
    // printf("hi from scheduler\n");

    key_t key = ftok("key", 'p');
    msgqid = msgget(key, 0666 | IPC_CREAT); // this message queue will comunicate process generator with scheduler

    if (msgqid == -1)
    {
        perror("Error in creating message queue");
        exit(-1);
    }

    algorithm = atoi(argv[1]); // initializing variables
    quantum = atoi(argv[2]);
    numofProcesses = atoi(argv[3]);

    // printf("%d ,%d ,%d ", algorithm, quantum, numofProcesses);
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
    p->pid = fork();
    p->waitingTime = 0;
    if (p->pid != 0)
    {
        printf("the scheduler forked a process with pid=%d\n", p->pid);
        (*runPshmadd) = p->remainingTime;
    }
    if (p->pid == 0)
    {

        execl("./process.out", "process", NULL);
    }

    return p->pid;
}

void stopProcess(struct Process *p)
{
    p->lastStopTime = getClk();
    printf("the scheduler stoped process with pid=%d\n", p->pid);
    kill(p->pid, SIGSTOP);
    p->remainingTime = (*runPshmadd);
}

void contProcess(struct Process *p)
{
    p->waitingTime += getClk() - p->lastStopTime;
    printf("the scheduler sent continue signal to process with pid=%d\n", p->pid);
    (*runPshmadd) = p->remainingTime;
    kill(p->pid, SIGCONT);
}

//------------------------RR algorithm-----------------------------------//
void RR(int quantum)
{
    printf("executing RR algorithm\n");
    struct CircularQueue readyQueue; // queue to store arrived processes
    initializeCircularQueue(&readyQueue);
    int remainQuantum = quantum;
    int currentTime = getClk();

    //---creating processes (should be replaced by msgrcv)-----
    processRecv = (struct Process *)malloc(sizeof(struct Process));
    processRecv->remainingTime = 5;
    processRecv->pid = forkProcess(processRecv);
    stopProcess(processRecv);
    enqueueInCQ(&readyQueue, processRecv);
    processRecv = (struct Process *)malloc(sizeof(struct Process));
    processRecv->remainingTime = 4;
    processRecv->pid = forkProcess(processRecv);
    stopProcess(processRecv);
    enqueueInCQ(&readyQueue, processRecv);
    RunningProcess = NULL;
    numofProcesses = 2;
    //-------------------till here-----------------------

    while (getClk() == 0)
        ;
    while (1)
    {
        if (terminatedProcessesNum == numofProcesses) // if all processes are terminated
            break;

        if (RunningProcess == NULL)
        { // there is No valid running process
            if (!isEmpty(readyQueue))
            {
                remainQuantum = quantum;
                RunningProcess = dequeueOFCQ(&readyQueue);
                //(*runPshmadd) = RunningProcess->remainingTime;
                contProcess(RunningProcess);
                RunningProcess->state = running;
            }
        }
        else if (RunningProcess->remainingTime == 0 || remainQuantum == 0)
        {
            if (RunningProcess->remainingTime == 0) // terminate the process
            {
                printf("process with pid=%d terminated\n", RunningProcess->pid);
                printf("waiting time=%d\n",RunningProcess->waitingTime);
                RunningProcess->state = terminated;
                terminatedProcessesNum++;
                RunningProcess = NULL;
            }
            else
            {                                  // switch processes
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
                //(*runPshmadd) = RunningProcess->remainingTime;
                contProcess(RunningProcess); // send cont signal to this process
            }
            else
                RunningProcess = NULL;
        }
        if (RunningProcess != NULL && RunningProcess->remainingTime > 0 && remainQuantum > 0)
        {
            RunningProcess->remainingTime = (*runPshmadd);
            remainQuantum -= 1;
        }
        // if (arrivedProcessesNum < numofProcesses)
        // {
        //     printf("recv block\n");                              //check if there is any recieved processes
        //     int val = msgrcv(msgqid, &processRecv, sizeof(struct Process),0,!IPC_NOWAIT);
        //     if (val == -1)
        //     {
        //         perror("Error in Receiving");
        //         break;
        //     }
        //     else{
        //         printf("RR: i received a process %d\n",processRecv.id);
        //         forkProcess(&processRecv);
        //         stopProcess(&processRecv);
        //         enqueueInCQ(&readyQueue,&processRecv);
        //     }
        // }
        while (currentTime == getClk())
            ;                   // waiting till the next clk
        currentTime = getClk(); // update time
        printf("%d\n", currentTime);
    }
}

//---------------------HPF---------------------------------------------
void HPF()
{

}
