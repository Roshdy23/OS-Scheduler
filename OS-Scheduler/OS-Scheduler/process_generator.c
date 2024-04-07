#include "headers.h"
#include <string.h>
struct process* processes= NULL;

int msgqid;

int numofProcesses=0;

int algorithm=-1;
int qtm=-1;

int scheduler_id,clk_id;

void clearResources(int signum)
{
  //TODO Clears all resources in case of interruption

     msgctl(msgqid, IPC_RMID, NULL);
   
    kill(scheduler_id, SIGKILL);
    kill(clk_id, SIGKILL);
    kill(getpid(), SIGKILL);
}

void readFile()
{
    FILE *file;
    char line[100];

    file = fopen("processes.txt", "r");
    if (file == NULL)
    {
        printf("Error in opening the file .");
        return;
    }

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "#", 1) == 0)
        {
            continue; // Skip comment lines
        }
        numofProcesses++;
    }
    
    processes=(struct process *)malloc(numofProcesses * sizeof(struct process));

    struct process* temp = processes;
    

    int i=0;

    rewind(file); // to start from the begining of the file ahain after i knew the num of processes

      while (fgets(line, sizeof(line), file))
     {
        if (strncmp(line, "#", 1) == 0)
        {
            continue; // Skip comment lines
        }

        int id,arrival,runtime,priority;
        sscanf(line, "%d %d %d %d", &id, &arrival, &runtime, &priority);

        (processes + i)->id = id;

        (processes + i)->arrival_time=arrival;

        (processes + i)->runtime= runtime;

        (processes + i)->priority=priority;

        i++;


    }

    fclose(file);

}


void choose_scheduling_algo()
{
    printf("1-HPF\n");
    printf("2-RR\n");
    printf("3-STRN\n");
    
    printf("Please choose the number of the scheduling algorithm you want. \n");

   

    int ok=1;

    while(ok)
    {

    scanf("%d",&algorithm);

    if(algorithm==1)
    {
       ok=0;


    }
    else if(algorithm ==2)
    {
        ok=0;

        printf("please enter the quantum for round robin algorithm\n");

        scanf("%d",&qtm);
    }
    else if(algorithm==3)
    {
            ok=0;
    }
    else
    {
         printf("Please Enter a valid choice of the given numbers\n");
    }
    }

    return ;
}

void create_scheduler()
{

    scheduler_id= fork();

    if(scheduler_id==-1)
    {
        printf("error in creating scheduler\n");
        return;
    }

    if(scheduler_id==0)
    {
    char algstr[20];
    char qtmstr[20];
    char numofp[20];
    sprintf(algstr,"%d",algorithm);
    sprintf(qtmstr,"%d",qtm);
    sprintf(numofp,"%d",numofProcesses);
    execl("./scheduler.out", "scheduler.out", algorithm, qtm, numofProcesses, NULL);
    }
}

void create_clock()
{

    clk_id=fork();

    if(clk_id==-1)
    {
        printf("error in creating clock\n");
        return;
    }
    if(clk_id==0)
    execl("./clk.out", "clk.out", NULL);
}

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    readFile();// 1. Read the input files. (done)

    choose_scheduling_algo();   
    create_clock();                                     // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any. (done)
    create_scheduler();                                 // 3. Initiate and create the scheduler and clock processes.
                                                        // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    // int x = getClk();
    // printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources

        key_t key = ftok("key", 'p');
        msgqid = msgget(key, 0666 | IPC_CREAT);// this message queue will comunicate process generator with scheduler

        int it=0;

        while(it<numofProcesses)
        {
            
            while(it<numofProcesses&&(processes+it)->arrival_time==getClk())
            {   
                 msgsnd(msgqid, &processes[it], sizeof(struct process), 0);

                it++;
            }

        }
    destroyClk(true);
}

