#include "headers.h"

void RR(int quantum);

void HPF();

void SRTN();

void perf();

double squareRoot(double n);


int quantum;
struct CircularQueue readyQueue; // queue to store arrived processes

double TotalWTA, TotalWait, TotalRun, TotalTime;




int numofProcesses;             // total number of processes comming from generator
int recievedProcessesNum = 0;   // number of arrived processes
int terminatedProcessesNum = 0; // total number of terminated processes
int algorithm = -1;
struct Process *RunningProcess = NULL;
struct Process *min = NULL;
struct Process *recievedProcesses;
int msgqid, sigshmid;
double WtaT[500];
int *sigshmaddr;
int runPshmid; // shared memory for the remain time of the running process
int *runPshmadd;


int idealTime = 0;
int prevTime;

//-------------------------------main function---------------------------//
int main(int argc, char *argv[]) {
    FILE *schedulerlog = fopen("scheduler.log", "w");
    fclose(schedulerlog);
    FILE *schedulerperf = fopen("scheduler.perf", "w");
    fclose(schedulerperf);
    initClk();
    TotalTime = TotalWTA = TotalWait = 0;
    TotalRun = 0;
    while (getClk() == 0);
    printf("hi from scheduler\n");

    initializeCircularQueue(&readyQueue);
    key_t key = ftok("key", 'p');
    msgqid = msgget(key, 0666 | IPC_CREAT); // this message queue will comunicate process generator with scheduler

    if (msgqid == -1) {
        perror("Error in creating message queue");
        exit(-1);
    }

    algorithm = atoi(argv[1]);
    quantum = atoi(argv[2]);
    numofProcesses = atoi(argv[3]);

    //    TODO implement the scheduler :)

    key_t shmkey = ftok("key", 'r'); // shared memory for the remain time of the running process
    runPshmid = shmget(shmkey, 4, 0666 | IPC_CREAT);
    runPshmadd = (int *) shmat(runPshmid, (void *) 0, 0);

    switch (algorithm) {
        case 1:
            HPF();
            break;
        case 2:
            RR(quantum);
            break;

        case 3:
            SRTN();
            break;
    }

    // upon termination release the clock resources.
    // destroyClk(true);
    perf();
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
    if (p->pid == 0) {
        printf("forked\n");
        execl("./process.out", "./process.out", NULL);
    } else {
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

//------------------------ RR algorithm -----------------------------------//
void RR(int quantum) {
    printf("executing RR algorithm\n");
    int remainQuantum = quantum;

    recievedProcesses = (struct Process *) malloc(numofProcesses * sizeof(struct Process));
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
        if (RunningProcess == NULL) {
            if (!isEmpty(readyQueue)) {
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
            } else
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
void HPF() {
    printf("executing HPF algorithm\n");

    struct priority_Queue readyQueue;


    recievedProcesses = (struct Process *) malloc(numofProcesses * sizeof(struct Process));
    struct Process *recv;

    int currentTime;

    while (true) {
        // initialize the time
        currentTime = getClk();

        // if all processes are terminated
        if (terminatedProcessesNum == numofProcesses) {
            TotalTime = currentTime - 2;
            break;
        }

        // check if there is any recieved processes
        while (currentTime == getClk())
            if (recievedProcessesNum < numofProcesses) {
                int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0,
                                 IPC_NOWAIT);
                while (val != -1) {
                    recv = &recievedProcesses[recievedProcessesNum];
                    forkProcess(recv);
                    kill(recv->pid, SIGSTOP);
                    priority_enqueue(&readyQueue, recv, 0);
                    recievedProcessesNum++;
                    val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0,
                                 IPC_NOWAIT);
                }
            }

        // running new process

        if (RunningProcess && RunningProcess->remainingTime == 0) {
            // noyify process termination
            if (RunningProcess->remainingTime == 0) {
                printf("process with id=%d terminated with %d\n", RunningProcess->id, RunningProcess->remainingTime);
                RunningProcess->state = terminated;
                terminatedProcessesNum++;
                RunningProcess->finishtime = currentTime;
                Outp(3, currentTime, RunningProcess);
                RunningProcess = NULL;
            }
        }
        if (!RunningProcess) {
            if (!priority_isempty(&readyQueue)) {

                RunningProcess = priority_dequeue(&readyQueue);
                contProcess(RunningProcess);
                RunningProcess->state = running;
                RunningProcess->starttime = currentTime;
                Outp(0, currentTime, RunningProcess);
            }
        }

        // get remainig time
        if (RunningProcess && RunningProcess->remainingTime > 0) {
            RunningProcess->remainingTime = (*runPshmadd);

        }

        printf("%d\n", currentTime);
    }

}

//---------------------------------------------------------------//
//------------------SRTN Algorithm----------------//
void SRTN() {
    printf("executing SRTN algorithm\n");

    struct priority_Queue readyQueue;


    recievedProcesses = (struct Process *) malloc(numofProcesses * sizeof(struct Process));
    struct Process *recv;

    int currentTime;

    while (true) {
        // initialize the time
        currentTime = getClk();

        // if all processes are terminated
        if (terminatedProcessesNum == numofProcesses) {
            TotalTime = currentTime - 1;
            break;
        }

        // check if there is any recieved processes
        while (currentTime == getClk())
            if (recievedProcessesNum < numofProcesses) {
                int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0,
                                 IPC_NOWAIT);
                while (val != -1) {
                    recv = &recievedProcesses[recievedProcessesNum];
                    forkProcess(recv);
                    kill(recv->pid, SIGSTOP);
                    recv->starttime = -1;
                    priority_enqueue(&readyQueue, recv, 1);
                    recievedProcessesNum++;
                    val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0,
                                 IPC_NOWAIT);
                }
            }
        // running new process
        if (RunningProcess == NULL) {
            if (!priority_isempty(&readyQueue)) {
                RunningProcess = priority_dequeue(&readyQueue);
                contProcess(RunningProcess);
                RunningProcess->state = running;
                if (RunningProcess->starttime == -1) {
                    RunningProcess->starttime = currentTime;
                    Outp(0, currentTime, RunningProcess);
                } else {
                    Outp(1, currentTime, RunningProcess);
                }
            }
        } else if (!priority_isempty(&readyQueue)) {
            min = priority_peek(&readyQueue);
            if (min->remainingTime < RunningProcess->remainingTime) {
                Outp(2, currentTime, RunningProcess);
                RunningProcess->state = ready;
                priority_enqueue(&readyQueue, RunningProcess, 1);
                stopProcess(RunningProcess);
                RunningProcess = min;
                priority_dequeue(&readyQueue);
                RunningProcess->state = running;
                (*runPshmadd) = RunningProcess->remainingTime;
                contProcess(RunningProcess);
                if (RunningProcess->starttime == -1) {
                    RunningProcess->starttime = currentTime;
                    Outp(0, currentTime, RunningProcess);
                } else {
                    Outp(1, currentTime, RunningProcess);
                }
            }
        }
        if (RunningProcess->remainingTime == 0) {
            // notify process termination
            RunningProcess->finishtime = currentTime;
            Outp(3, currentTime, RunningProcess);
            RunningProcess->state = terminated;
            terminatedProcessesNum++;
            RunningProcess = NULL;
        }
        if (RunningProcess == NULL) {
            if (!priority_isempty(&readyQueue)) {
                RunningProcess = priority_dequeue(&readyQueue);
                contProcess(RunningProcess);
                RunningProcess->state = running;
                if (RunningProcess->starttime == -1) {
                    RunningProcess->starttime = currentTime;
                    Outp(0, currentTime, RunningProcess);
                } else {
                    Outp(1, currentTime, RunningProcess);
                }
            }
        }
        // get remaining time
        if (RunningProcess != NULL && RunningProcess->remainingTime > 0) {
            RunningProcess->remainingTime = (*runPshmadd);

        }
        printf("%d\n", currentTime);
    }
}