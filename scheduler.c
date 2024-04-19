#include "headers.h"
#include<math.h>

void perf();
double squareRoot(double n);
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
int idealtime;
FILE*pf;
//-------------------------------main function---------------------------//
int main(int argc, char *argv[])
{
    idealtime=0;
    pf=fopen("scheduler.log","w");
    if(pf==NULL)
         {printf("error in opening file");}
    initClk();
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
//------------Output Function-------------//
void Outp(int op, int time, struct Process *p) {
    FILE *schedulerlog = fopen("scheduler.log", "a");
    if (op == 0) {
        fprintf(schedulerlog, "At time %d process %d started arr %d total %d remain %d wait %d", time, p->id,
                p->arrival_time,
                p->runtime, p->remainingTime,
                time - p->arrival_time - p->runtime + p->remainingTime);
    }
    if (op == 1) {
        fprintf(schedulerlog, "At time %d process %d resumed arr %d total %d remain %d wait %d", time, p->id,
                p->arrival_time,
                p->runtime, p->remainingTime,
                time - p->arrival_time - p->runtime + p->remainingTime);
    }
    if (op == 2) {
        fprintf(schedulerlog, "At time %d process %d stopped arr %d total %d remain %d wait %d", time, p->id,
                p->arrival_time,
                p->runtime, p->remainingTime,
                time - p->arrival_time - p->runtime + p->remainingTime);
    }
    if (op == 3) {
        double x = p->finishtime - p->arrival_time;
        fprintf(schedulerlog, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2lf", time,
                p->id,
                p->arrival_time, p->runtime,
                p->remainingTime,
                time - p->arrival_time - p->runtime + p->remainingTime, p->finishtime - p->arrival_time,
                (x) / p->runtime);
        TotalWait += time - p->arrival_time - p->runtime + p->remainingTime;
        TotalWTA += (x) / p->runtime;
        TotalRun += p->runtime;
        WtaT[p->id] = (x) / p->runtime;
    }
    fprintf(schedulerlog, "\n");
    fclose(schedulerlog);
}

//-------------scheduler.perf output---------//

void perf() {
    double std = 0;

    TotalWTA /= numofProcesses;
    TotalWait /= numofProcesses;
    FILE *schedulerperf = fopen("scheduler.perf", "a");
    TotalRun /= TotalTime;
    TotalRun *= 100;
    fprintf(schedulerperf, "CPU Utilization = %.2lf%%\n", TotalRun);
    fprintf(schedulerperf, "Avg WTA = %.2lf\n", TotalWTA);
    fprintf(schedulerperf, "Avg Waiting = %.2lf\n", TotalWait);
    for (int i = 1; i <= numofProcesses; ++i) {
        std += (WtaT[i] - TotalWTA)*(WtaT[i] - TotalWTA);
    }
    std = squareRoot(std);
    fprintf(schedulerperf, "Std WTA = %.2lf\n", std);
    fclose(schedulerperf);

}

//-------------Square root function to calculate std-----------'

double squareRoot(double n) {
    double x = n;
    double y = 1;
    double epsilon = 0.000001;

    while (x - y > epsilon) {
        x = (x + y) / 2;
        y = n / x;
    }
    return x;
}

//-----------------------------processes operation functions--------------------//
int forkProcess(struct Process *p)
{
    p->pid = 0;
    p->lastStopTime=getClk();
    p->waitingTime = 0;
    p->remainingTime = p->runtime;
    p->pid = fork();

    if (p->pid == 0)
    {
        execl("./process.out", "./process.out", NULL);
    }
    else
    {
        // printf("the scheduler forked a process with id=%d at %d\n", p->id, getClk());
        return p->pid;
    }
}

void stopProcess(struct Process *p)
{
    p->lastStopTime = getClk();
    // printf("the scheduler stoped process with id=%d\n", p->id);
    kill(p->pid, SIGSTOP);
    p->remainingTime = (*runPshmadd);
}

void contProcess(struct Process *p)
{
    p->waitingTime += getClk() - p->lastStopTime;
    // printf("the scheduler sent continue signal to process with id=%d\n", p->id);
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

    int currentTime;
    int prevtime=1;
    bool firsttime=1;
    while (getClk() == 0);
    
    while (terminatedProcessesNum < numofProcesses)
    {
        currentTime = getClk();
        usleep(100);
        // if all processes are terminated
        int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);
        if (val != -1)
        {
            recv = &recievedProcesses[recievedProcessesNum];
            forkProcess(recv);
            kill(recv->id,SIGCONT);
            raise(SIGSTOP);
            usleep(100); //wait 10microseconds
            kill(recv->id,SIGCONT);
            enqueueInCQ(&readyQueue, recv);
            recievedProcessesNum++;
        }
        if(currentTime==1&&!isEmpty(readyQueue)&&firsttime)
        {
            firsttime=0;
            remainQuantum = quantum;
            RunningProcess = dequeueOFCQ(&readyQueue);
            contProcess(RunningProcess);
            RunningProcess->state = running;
            fprintf(pf,"At time %d process %d started arr %d total %d ramain %d wait %d\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->remainingTime,RunningProcess->waitingTime);
        }
          // start of the program
        if(currentTime!=prevtime)
        {
            prevtime=currentTime;
            currentTime=getClk();
                         // get remainig time & reduce the remaining quantum
            if (RunningProcess != NULL && RunningProcess->remainingTime > 0 && remainQuantum > 0)
            {
                kill(RunningProcess->pid,SIGCONT);
                raise(SIGSTOP);
                usleep(100); //wait 10microseconds
                kill(RunningProcess->id,SIGCONT);
                RunningProcess->remainingTime = (*runPshmadd);
                remainQuantum -= 1;
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
                    fprintf(pf,"At time %d process %d started arr %d total %d ramain %d wait %d\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->remainingTime,RunningProcess->waitingTime);
                }
                else
                {
                    idealtime++;
                }
            }
            // running process checks
            else if (RunningProcess->remainingTime == 0 || remainQuantum == 0)
            {
                // noyify process termination
                if (RunningProcess->remainingTime == 0)
                {
                    // printf("process with id=%d terminated with %d\n", RunningProcess->id, RunningProcess->remainingTime);
                    RunningProcess->state = terminated;
                    terminatedProcessesNum++;
                     int TA=currentTime-RunningProcess->arrival_time;
                    double WTA=(TA/(double)RunningProcess->runtime);
                    fprintf(pf,"At time %d process %d finished arr %d total %d ramain 0 wait %d TA %.d WTA %.2f\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->waitingTime,TA,WTA);
                    RunningProcess = NULL;
                    // printf("the process terminated\n");
                // 
                }
                // switch processes
                else
                {
                    RunningProcess->state = ready; // change state to ready
                    enqueueInCQ(&readyQueue, RunningProcess);
                    stopProcess(RunningProcess); // send stop signal to this process
                    fprintf(pf,"At time %d process %d stopped arr %d total %d remain %d wait %d\n",
                    currentTime, RunningProcess->id, RunningProcess->arrival_time,
                    RunningProcess->runtime, RunningProcess->remainingTime,
                    RunningProcess->waitingTime);
                    RunningProcess = NULL;
                    // printf("stopped\n");
                }
                if (!isEmpty(readyQueue))
                {
                    printf("Context switching\n");
                    remainQuantum = quantum;
                    RunningProcess = dequeueOFCQ(&readyQueue);
                    RunningProcess->state = running;
                    (*runPshmadd) = RunningProcess->remainingTime;
                    contProcess(RunningProcess);
                    fprintf(pf,"At time %d process %d started arr %d total %d ramain %d wait %d\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->remainingTime,RunningProcess->waitingTime);
                }
                else
                {
                    idealtime++;
                }  
            }
        }
    }
}

//---------------------HPF---------------------------------------------
void HPF()
{
 printf("executing HPF algorithm\n");

 struct priority_Queue readyQueue;
 
   

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
                    priority_enqueue(&readyQueue, recv);
                    recievedProcessesNum++;

                    if (!RunningProcess)
                    {
                        
                        RunningProcess = priority_dequeue(&readyQueue);
                        contProcess(RunningProcess);
                        RunningProcess->state = running;
                    }

                    val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);
                }
            }

        // running new process
        if (RunningProcess == NULL)
        {
            if (!priority_isempty(&readyQueue))
            {
               
                RunningProcess = priority_dequeue(&readyQueue);
                contProcess(RunningProcess);
                RunningProcess->state = running;
            }
        }
        // running process checks
        else if (RunningProcess->remainingTime == 0 )
        {
            // noyify process termination
            if (RunningProcess->remainingTime == 0)
            {
                printf("process with id=%d terminated with %d\n", RunningProcess->id, RunningProcess->remainingTime);
                RunningProcess->state = terminated;
                terminatedProcessesNum++;
                RunningProcess = NULL;
            }
            if (!priority_isempty(&readyQueue))
            {
                printf("Context switching\n");
               
                RunningProcess = priority_dequeue(&readyQueue);
                RunningProcess->state = running;
                (*runPshmadd) = RunningProcess->remainingTime;
                contProcess(RunningProcess);
            }
            else
                RunningProcess = NULL;
        }
        // get remainig time 
        if (RunningProcess != NULL && RunningProcess->remainingTime > 0)
        {
            RunningProcess->remainingTime = (*runPshmadd);
           
        }

        printf("%d\n", currentTime);
    }
    
    
}
//---------------------------------------------------------------//