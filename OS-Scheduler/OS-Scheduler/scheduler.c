#include "headers.h"



#include <math.h>



// #include<buddysystem.c>



void RR(int quantum);


void HPF();



void SRTN();



void perf();



int quantum;



struct Queue readyQueue; // queue to store arrived processes



double squareRoot(double n);



double TotalWTA, TotalWait, TotalRun, TotalTime;



double WtaT[500];



struct Process *min = NULL;



int numofProcesses; // total number of processes comming from generator



int recievedProcessesNum = 0; // number of arrived processes



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

struct RRqueue* RRmemorylist;



void clearsemaphores()



{



    if (semctl(semm1, 0, IPC_RMID) == -1)

    {



        perror("Error removing semaphore");



        exit(EXIT_FAILURE);

    }



    if (semctl(semm2, 0, IPC_RMID) == -1)

    {



        perror("Error removing semaphore");



        exit(EXIT_FAILURE);

    }

    else

    {



        printf("Semaphore removed successfully\n");

    }

}



//-------------------------------main function---------------------------//



int main(int argc, char *argv[])



{



    RRmemorylist=createRRqueue();



    // create semaphores to handle process and scheduler synchronization



    union Semun semun;



    semm1 = semget('5', 1, 0666 | IPC_CREAT);



    semm2 = semget('6', 1, 0666 | IPC_CREAT);



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



    FILE *memorylog = fopen("memory.log", "w");



    fclose(schedulerperf);



    fclose(schedulerlog);



    fclose(memorylog);



    initClk();



    initmemory();



    initializeQueue(&readyQueue);



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



    perf();



    destroyClk(false);



    clearsemaphores();



    if (shmctl(runPshmid, IPC_RMID, NULL) == -1)

    {



        perror("error while clearing the shared memory");

    }



    execl("./image_generator.out", "image_generator.out", NULL);

}



//-----------------------------processes operation functions--------------------//



int forkProcess(struct Process *p)



{



    p->pid = 0;



    p->lastStopTime = getClk();



    p->waitingTime = 0;



    p->remainingTime = p->runtime;



    p->starttime = -1;



    p->pid = fork();



    if (p->pid == 0)



    {



        execl("./process.out", "./process.out", NULL);

    }



    else



    {



        return p->pid;

    }

}



void stopProcess(struct Process *p)



{



    p->lastStopTime = getClk();



    kill(p->pid, SIGSTOP);



    p->remainingTime = (*runPshmadd);

}



void contProcess(struct Process *p)



{



    p->waitingTime += getClk() - p->lastStopTime;



    (*runPshmadd) = p->remainingTime;



    kill(p->pid, SIGCONT);

}



//------------Output Function-------------//



void Outp(int op, int time, struct Process *p)

{



    FILE *schedulerlog = fopen("scheduler.log", "a");



    if (op == 0)

    {



        fprintf(schedulerlog, "At time %d process %d started arr %d total %d remain %d wait %d", time, p->id,



                p->arrival_time,



                p->runtime, p->remainingTime,



                time - p->arrival_time - p->runtime + p->remainingTime);

    }



    if (op == 1)

    {



        fprintf(schedulerlog, "At time %d process %d resumed arr %d total %d remain %d wait %d", time, p->id,



                p->arrival_time,



                p->runtime, p->remainingTime,



                time - p->arrival_time - p->runtime + p->remainingTime);

    }



    if (op == 2)

    {



        fprintf(schedulerlog, "At time %d process %d stopped arr %d total %d remain %d wait %d", time, p->id,



                p->arrival_time,



                p->runtime, p->remainingTime,



                time - p->arrival_time - p->runtime + p->remainingTime);

    }



    if (op == 3)

    {



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



void perf()

{



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



    for (int i = 1; i <= numofProcesses; ++i)

    {



        std += (WtaT[i] - TotalWTA) * (WtaT[i] - TotalWTA);

    }



    std /= numofProcesses;



    std = sqrt(std);



    fprintf(schedulerperf, "Std WTA = %.2lf\n", std);



    fclose(schedulerperf);

}

//------------------------ RR algorithm -----------------------------------//

void RR(int quantum)

{

    printf("executing RR algo\n");

    int remainQuantum = quantum;

    recievedProcesses = (struct Process *)malloc(numofProcesses * sizeof(struct Process));

    struct Process *recv;

    int currentTime;

    int prevtime = 0;

    while (terminatedProcessesNum < numofProcesses)

    {



        currentTime = getClk();

        usleep(200); // just to be sure the process generator arrives and send the process info



        // recieve process and put it in recievedProcesses array

        int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);



        if (val != -1)

        {

            recv = &recievedProcesses[recievedProcessesNum];

            recievedProcessesNum++;

            int result = allocatememory(recv, getClk());

            if (result == 1)

            {



                printmemory(getClk(), recv->size, recv->id, recv->start, recv->start + recv->size, 1);

                forkProcess(recv);



                // to handler synchronization between process

                kill(recv->id, SIGCONT);

                TotalRun += recv->runtime;

                raise(SIGSTOP);

                usleep(100); // wait 10microseconds

                kill(recv->id, SIGCONT);

                enqueue(&readyQueue, recv);

            }



            else

            {



                printf("no space in memory\n");

                putinRRlist(RRmemorylist,recv);

            }

        }



        // start of the program

        if (currentTime != prevtime)

        {

            if (!RunningProcess)

                idealtime++;



            prevtime = currentTime;

            currentTime = getClk();



            // get remainig time & reduce the remaining quantum

            if (RunningProcess != NULL && RunningProcess->remainingTime > 0 && remainQuantum > 0)

            {

                up(semm2);

                down(semm1);

                RunningProcess->remainingTime = (*runPshmadd);

                remainQuantum -= 1;

            }



            // running new process

            if (RunningProcess == NULL)

            {

                if (!isEmpty(readyQueue))

                {

                    remainQuantum = quantum;

                    RunningProcess = dequeue(&readyQueue);

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

                    waitpid(RunningProcess->pid, NULL, 0);

                    RunningProcess->state = terminated;

                    freememory(RunningProcess->start, RunningProcess->size);

                    printmemory(getClk(), RunningProcess->size, RunningProcess->id, RunningProcess->start, RunningProcess->start + RunningProcess->size, 2);

                    struct Process *p = getfromRRlist(RRmemorylist);

                    while (p != NULL)

                    {

                        int result = allocatememory(p, getClk());

                        if (result == 1)

                        {

                            printmemory(getClk(), p->size, p->id, p->start, p->start + p->size, 1);

                            forkProcess(p);

                            // to handler synchronization between process

                            kill(p->id, SIGCONT);

                            TotalRun += p->runtime;

                            raise(SIGSTOP);

                            usleep(100); // wait 10microseconds

                            kill(p->id, SIGCONT);

                            enqueue(&readyQueue, p);

                        }

                        else

                        {



                            printf("no space in memory\n");

                            putinRRlist(RRmemorylist,p);

                            break;

                        }

                        p = getfromRRlist(RRmemorylist);

                    }

                    terminatedProcessesNum++;

                    RunningProcess->finishtime = currentTime;

                    Outp(3, currentTime, RunningProcess);

                    RunningProcess = NULL;

                }



                // switch processes

                else

                {

                    RunningProcess->state = ready;

                    enqueue(&readyQueue, RunningProcess);

                    stopProcess(RunningProcess);

                    Outp(2, currentTime, RunningProcess);

                    RunningProcess = NULL;

                }



                if (!isEmpty(readyQueue))

                {

                    // printf("Context switching\n");

                    remainQuantum = quantum;

                    RunningProcess = dequeue(&readyQueue);

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

    free(recievedProcesses);

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



    int prevtime = 0;



    while (terminatedProcessesNum < numofProcesses)



    {



        currentTime = getClk();



        usleep(100);



        int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);



        if (val != -1)



        {



            recv = &recievedProcesses[recievedProcessesNum];



            recievedProcessesNum++;



            int result = allocatememory(recv, getClk());



            if (result == 1)



            {



                printmemory(getClk(), recv->size, recv->id, recv->start, recv->start + recv->size, 1);



                forkProcess(recv);



                // to handler synchronization between process



                kill(recv->id, SIGCONT);



                TotalRun += recv->runtime;



                raise(SIGSTOP);



                usleep(100); // wait 10microseconds



                kill(recv->id, SIGCONT);



                priority_enqueue(&readyQueue2, recv, 0);

            }



            else

            {



                printf("no space in memory\n");



                // putinRRlist(recv);

            }

        }



        // start of the program



        if (currentTime != prevtime)



        {



            prevtime = currentTime;



            currentTime = getClk();



            // get remainig time & reduce the remaining quantum



            if (RunningProcess != NULL && RunningProcess->remainingTime > 0)



            {



                up(semm2);



                down(semm1);



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



                    Outp(0, currentTime, RunningProcess);

                }



                else



                {



                    idealtime++;

                }

            }



            // running process checks



            else if (RunningProcess->remainingTime == 0)



            {



                freememory(RunningProcess->start, RunningProcess->size);



                printmemory(getClk(), RunningProcess->size, RunningProcess->id, RunningProcess->start, RunningProcess->start + RunningProcess->size, 2);



                struct Process *p = getfromRRlist(RRmemorylist);



                while (p != NULL)



                {



                    int result = allocatememory(p, getClk());



                    if (result == 1)



                    {



                        printmemory(getClk(), p->size, p->id, p->start, p->start + p->size, 1);



                        forkProcess(p);



                        // to handler synchronization between process



                        kill(p->id, SIGCONT);



                        TotalRun += p->runtime;



                        raise(SIGSTOP);



                        usleep(100); // wait 10microseconds



                        kill(p->id, SIGCONT);



                        priority_enqueue(&readyQueue2, p, 0);

                    }



                    else

                    {



                        printf("no space in memory\n");

                        // putinRRlist(p);

                        break;

                    }



                    // p = getfromRRlist();

                }



                // noyify process termination



                RunningProcess->state = terminated;



                terminatedProcessesNum++;



                RunningProcess->finishtime = currentTime;



                Outp(3, currentTime, RunningProcess);



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



                    Outp(0, currentTime, RunningProcess);

                }



                else



                {



                    idealtime++;

                }

            }

        }

    }

    free(recievedProcesses);

}



//---------------------------------------------------------------//



//------------------SRTN Algorithm----------------//



void SRTN()

{



    printf("executing SRTN algorithm\n");



    recievedProcesses = (struct Process *)malloc(numofProcesses * sizeof(struct Process));



    struct Process *recv;



    struct priority_Queue readyQueue;



    initializePriorityQueue(&readyQueue);



    int currentTime;



    int prevtime = 1;



    bool firsttime = 1;



    while (terminatedProcessesNum < numofProcesses)

    {



        currentTime = getClk();



        usleep(100);

        int val = msgrcv(msgqid, &recievedProcesses[recievedProcessesNum], sizeof(struct Process), 0, IPC_NOWAIT);



        if (val != -1)

        {

            recv = &recievedProcesses[recievedProcessesNum];



            recievedProcessesNum++;



            recv->starttime = -1;



            int result = allocatememory(recv, getClk());



            if (result == 1)



            {



                printmemory(getClk(), recv->size, recv->id, recv->start, recv->start + recv->size, 1);



                forkProcess(recv);



                // to handler synchronization between process



                kill(recv->id, SIGCONT);



                TotalRun += recv->runtime;



                raise(SIGSTOP);



                usleep(100); // wait 10microseconds



                kill(recv->id, SIGCONT);



                priority_enqueue(&readyQueue, recv, 1);

            }

            else

            {



                printf("no space in memory\n");

                // putinRRlist(recv);

            }

        }



        // start of the program



        if (currentTime != prevtime)

        {



            if (RunningProcess && RunningProcess->remainingTime == 0)

            {



                freememory(RunningProcess->start, RunningProcess->size);



                printmemory(getClk(), RunningProcess->size, RunningProcess->id, RunningProcess->start, RunningProcess->start + RunningProcess->size, 2);



                struct Process *p = getfromRRlist(RRmemorylist);



                while (p != NULL)

                {



                    int result = allocatememory(p, getClk());



                    if (result == 1)



                    {



                        printmemory(getClk(), p->size, p->id, p->start, p->start + p->size, 1);



                        forkProcess(p);



                        // to handler synchronization between process



                        kill(p->id, SIGCONT);



                        TotalRun += p->runtime;



                        raise(SIGSTOP);



                        usleep(100); // wait 10microseconds



                        kill(p->id, SIGCONT);



                        priority_enqueue(&readyQueue, p, 1);

                    }



                    else

                    {



                        printf("no space in memory\n");

                        // putinRRlist(p);

                        break;

                    }



                    // p = getfromRRlist();

                }



                // notify process termination



                RunningProcess->finishtime = currentTime;



                Outp(3, currentTime, RunningProcess);



                RunningProcess->state = terminated;



                terminatedProcessesNum++;



                RunningProcess = NULL;

            }



            if (RunningProcess == NULL)

            {



                if (!priority_isempty(&readyQueue))

                {



                    RunningProcess = priority_dequeue(&readyQueue);



                    RunningProcess->state = running;



                    (*runPshmadd) = RunningProcess->remainingTime;



                    contProcess(RunningProcess);



                    if (RunningProcess->starttime == -1)

                    {



                        RunningProcess->starttime = currentTime;



                        Outp(0, currentTime, RunningProcess);

                    }

                    else

                    {



                        Outp(1, currentTime, RunningProcess);

                    }

                }

            }

            else if (!priority_isempty(&readyQueue))

            {



                min = priority_peek(&readyQueue);



                if (min->remainingTime < RunningProcess->remainingTime)

                {



                    Outp(2, currentTime, RunningProcess);



                    RunningProcess->state = ready;



                    priority_enqueue(&readyQueue, RunningProcess, 1);



                    stopProcess(RunningProcess);



                    RunningProcess = min;



                    priority_dequeue(&readyQueue);



                    RunningProcess->state = running;



                    (*runPshmadd) = RunningProcess->remainingTime;



                    contProcess(RunningProcess);



                    if (RunningProcess->starttime == -1)

                    {



                        RunningProcess->starttime = currentTime;



                        Outp(0, currentTime, RunningProcess);

                    }

                    else

                    {



                        Outp(1, currentTime, RunningProcess);

                    }

                }

            }



            // get remaining time



            if (RunningProcess != NULL && RunningProcess->remainingTime > 0)

            {



                up(semm2);



                down(semm1);



                RunningProcess->remainingTime--;

            }



            else if (!RunningProcess && terminatedProcessesNum != numofProcesses)

            {



                idealtime++;

            }



            prevtime = currentTime;



            currentTime = getClk();

        }

    }

}