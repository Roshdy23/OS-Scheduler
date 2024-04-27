#include "headers.h"
#include<math.h>
void RR(int quantum);
void HPF();
void SRTN();
void perf();
int quantum;
struct CircularQueue readyQueue; // queue to store arrived processes
double squareRoot(double n);
double TotalWTA, TotalWait, TotalRun, TotalTime;
double WtaT[500];
struct Process *min = NULL;


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
double idealtime = 0;
int semm1;
int semm2;
FILE*pf;
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
void clearsemaphores()
{
    if(semctl(semm1, 0, IPC_RMID) == -1) {
        perror("Error removing semaphore");
        exit(EXIT_FAILURE);
    }
     if(semctl(semm2, 0, IPC_RMID) == -1) {
        perror("Error removing semaphore");
        exit(EXIT_FAILURE);
    } else {
        printf("Semaphore removed successfully\n");
    }
}
//-------------------------------main function---------------------------//
int main(int argc, char *argv[])
{
        union Semun semun;

     semm1 = semget('5', 1, 0666 | IPC_CREAT);
     semm2 = semget('6', 1, 0666 | IPC_CREAT);
    // printf("%d\n",semm2);
    if (semm1 == -1 || semm2 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }

    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(semm1, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    if (semctl(semm2, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    TotalTime = TotalWTA = TotalWait = TotalRun = 0;
    FILE *schedulerlog = fopen("scheduler.log", "w");
    FILE *schedulerperf = fopen("scheduler.perf", "w");
    fclose(schedulerperf);
    fclose(schedulerlog);
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
    case 3:
        SRTN();
        break;
    }

    // upon termination release the clock resources.
    // destroyClk(true);
    perf();
    execl("./image_generator.out","image_generator.out",NULL);
}

//-----------------------------processes operation functions--------------------//
int forkProcess(struct Process *p)
{
    p->pid = 0;
    p->lastStopTime=getClk();
    p->waitingTime = 0;
    p->remainingTime = p->runtime;
    p->starttime=-1;
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
    TotalTime = TotalRun + idealtime;
    FILE *schedulerperf = fopen("scheduler.perf", "a");
    fprintf(schedulerperf, "Running time %.2lf Total Time %.2lf \n", TotalRun, TotalTime);
    TotalRun /= TotalTime;
    TotalRun *= 100;
    fprintf(schedulerperf, "CPU Utilization = %.2lf%%\n", TotalRun);
    fprintf(schedulerperf, "Avg WTA = %.2lf\n", TotalWTA);
    fprintf(schedulerperf, "Avg Waiting = %.2lf\n", TotalWait);
    for (int i = 1; i <= numofProcesses; ++i) {
        std += (WtaT[i] - TotalWTA) * (WtaT[i] - TotalWTA);
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
void RR(int quantum)
{
    printf("executing RR algorithm\n");
    // printf("%d\n",semm2);
    int remainQuantum = quantum;

    recievedProcesses = (struct Process *)malloc(numofProcesses * sizeof(struct Process));
    struct Process *recv;

    int currentTime;
    int prevtime=0;
    // while (getClk() == 0);
    
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
            TotalRun+=recv->runtime;
            raise(SIGSTOP);
            usleep(100); //wait 10microseconds
            kill(recv->id,SIGCONT);
            enqueueInCQ(&readyQueue, recv);
            recievedProcessesNum++;
        }
          // start of the program
        if(currentTime!=prevtime)
        {
            if (!RunningProcess)
                idealtime++;
            prevtime = currentTime;
            currentTime = getClk();
                         // get remainig time & reduce the remaining quantum
            if (RunningProcess != NULL && RunningProcess->remainingTime > 0 && remainQuantum > 0)
            {
                printf("scheduler:%d\n",currentTime);
                // printf("%d\n",semm2);
                up(semm2);
                down(semm1);
                // kill(RunningProcess->pid,SIGCONT);
                // raise(SIGSTOP);
                // usleep(100); //wait 10microseconds
                // kill(RunningProcess->id,SIGCONT);
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
                    if (RunningProcess->starttime == -1)
                    {
                        Outp(0, currentTime, RunningProcess);
                        RunningProcess->starttime = currentTime;
                }
                else
                        Outp(1, currentTime, RunningProcess);
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
                    RunningProcess->finishtime = currentTime;
                    Outp(3, currentTime, RunningProcess);
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
                    Outp(2, currentTime, RunningProcess);
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
                    if (RunningProcess->remainingTime == RunningProcess->runtime)
                        Outp(0, currentTime, RunningProcess);
                    else
                        Outp(1, currentTime, RunningProcess);
                }
            }
        }
    }
}

//---------------------HPF---------------------------------------------
void HPF()
{
   printf("executing HPF algorithm\n");
    
   struct priority_Queue readyQueue2;
   initializePriorityQueue(&readyQueue2);

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
            priority_enqueue(&readyQueue2,recv,0);
            recievedProcessesNum++;
        }
        if(currentTime==1&&!priority_isempty(&readyQueue2)&&firsttime)
        {
            firsttime=0;
            RunningProcess = priority_dequeue(&readyQueue2);
            contProcess(RunningProcess);
            RunningProcess->state = running;
            // fprintf(pf,"At time %d process %d started arr %d total %d ramain %d wait %d\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->remainingTime,RunningProcess->waitingTime);
            Outp(0,currentTime,RunningProcess);
     
        }
          // start of the program
        if(currentTime!=prevtime)
        {
            prevtime=currentTime;
            currentTime=getClk();
                         // get remainig time & reduce the remaining quantum
            if (RunningProcess != NULL && RunningProcess->remainingTime > 0)
            {
                  up(semm2);
                down(semm1);
                // kill(RunningProcess->pid,SIGCONT);
                // raise(SIGSTOP);
                // usleep(100); //wait 10microseconds
                // kill(RunningProcess->id,SIGCONT);
                RunningProcess->remainingTime = (*runPshmadd);
               
            }
            // running new process
            if (RunningProcess == NULL)
            {
                if (!priority_isempty(&readyQueue2))
                {
                    RunningProcess = priority_dequeue(&readyQueue2);
                    contProcess(RunningProcess);
                    RunningProcess->state = running;
                    // fprintf(pf,"At time %d process %d started arr %d total %d ramain %d wait %d\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->remainingTime,RunningProcess->waitingTime);
                      Outp(0,currentTime,RunningProcess);

                }
                else
                {
                    idealtime++;
                }
            }
            // running process checks
            else if (RunningProcess->remainingTime == 0 )
            {
                // noyify process termination
           
                    // printf("process with id=%d terminated with %d\n", RunningProcess->id, RunningProcess->remainingTime);
                    RunningProcess->state = terminated;
                    terminatedProcessesNum++;
                     int TA=currentTime-RunningProcess->arrival_time;
                    double WTA=(TA/(double)RunningProcess->runtime);
                    // fprintf(pf,"At time %d process %d finished arr %d total %d ramain 0 wait %d TA %.d WTA %.2f\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->waitingTime,TA,WTA);
                   RunningProcess->finishtime=currentTime;
                   Outp(3,currentTime,RunningProcess);
                    RunningProcess = NULL;
                    printf("the process terminated\n");
                     

                // 
                
                if (!priority_isempty(&readyQueue2))
                {
                    printf("Context switching\n");
                    RunningProcess = priority_dequeue(&readyQueue2);
                    RunningProcess->state = running;
                    (*runPshmadd) = RunningProcess->remainingTime;
                    contProcess(RunningProcess);
                    
                    
                    // fprintf(pf,"At time %d process %d started arr %d total %d ramain %d wait %d\n",currentTime,RunningProcess->id,RunningProcess->arrival_time,RunningProcess->runtime,RunningProcess->remainingTime,RunningProcess->waitingTime);
              Outp(0,currentTime,RunningProcess);

                }
                else
                {
                    idealtime++;
                }  
            }
        }
    }
}

//---------------------------------------------------------------//
//------------------SRTN Algorithm----------------//
void SRTN() {
    printf("executing SRTN algorithm\n");

    recievedProcesses = (struct Process *) malloc(numofProcesses * sizeof(struct Process));
    struct Process *recv;
    struct priority_Queue readyQueue;

    int currentTime;
    int prevtime = 0;
    bool firsttime = 1;
    while (getClk() == 0);

    while (terminatedProcessesNum < numofProcesses) {
        currentTime = getClk();
        usleep(100);
        // if all processes are terminated
        int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);
        if (val != -1) {
            recv = &recievedProcesses[recievedProcessesNum];
            forkProcess(recv);
            kill(recv->id, SIGCONT);
            raise(SIGSTOP);
            usleep(100); //wait 10microseconds
            kill(recv->id, SIGCONT);
            recv->starttime = -1;
            priority_enqueue(&readyQueue, recv, 1);
            TotalRun+=recv->runtime;
            recievedProcessesNum++;
        }
        // start of the program
        if (currentTime != prevtime) {
            if (RunningProcess && RunningProcess->remainingTime == 0) {
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
            // get remaining time
            if (RunningProcess != NULL && RunningProcess->remainingTime > 0) {
                // kill(RunningProcess->pid, SIGCONT);
                // raise(SIGSTOP);
                // usleep(100); //wait 10microseconds
                // kill(RunningProcess->id, SIGCONT);
                up(semm2);
                down(semm1);
                RunningProcess->remainingTime--;
            }
            else if (!RunningProcess && terminatedProcessesNum != numofProcesses){
                idealtime++;
            }
            prevtime = currentTime;
            currentTime = getClk();
        }
    }
}