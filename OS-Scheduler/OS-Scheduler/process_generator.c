#include "headers.h"

#include <string.h>

struct Process *processes = NULL;



int msgqid;



int numofProcesses = 0;



int algorithm = -1;

int qtm = -1;



int scheduler_id, clk_id, sigshmid;

int *sigshmaddr;



void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    msgctl(msgqid, IPC_RMID, NULL);
    free(processes);
    kill(scheduler_id, SIGKILL);
    kill(clk_id, SIGKILL);
    kill(getpid(), SIGKILL);
}



void readFile(char* filePath)

{

    FILE *file;

    char line[100];



    file = fopen(filePath, "r");

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



    processes = (struct Process *)malloc(numofProcesses * sizeof(struct Process));



    struct Process *temp = processes;



    int i = 0;



    rewind(file); // to start from the begining of the file ahain after i knew the num of processes



    while (fgets(line, sizeof(line), file))

    {

        if (strncmp(line, "#", 1) == 0)

        {

            continue; // Skip comment lines

        }
        int id, arrival, runtime, priority,memorysize;
        sscanf(line, "%d %d %d %d %d", &id, &arrival, &runtime, &priority,&memorysize);
        (processes + i)->id = id;
        (processes + i)->arrival_time = arrival;
        (processes + i)->runtime = runtime;
        (processes + i)->remainingTime = runtime;
        (processes + i)->priority = priority;
        (processes + i)->size = memorysize;
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



    int ok = 1;



    while (ok)

    {



        scanf("%d", &algorithm);



        if (algorithm == 1)

        {

            ok = 0;

        }

        else if (algorithm == 2)

        {

            ok = 0;



            printf("please enter the quantum for round robin algorithm\n");



            scanf("%d", &qtm);

        }

        else if (algorithm == 3)

        {

            ok = 0;

        }

        else

        {

            printf("Please Enter a valid choice of the given numbers\n");

        }

    }



    return;

}



void create_clock_and_scheduler()

{



    for (int i = 0; i < 2; i++)

    {

        int pidd = fork();



        if (pidd == -1)

            printf("error\n");



        if (pidd == 0)

        {

            if (i == 0)

            {

                execl("./clk.out", "clk.out", NULL);

            }

            else



            {

                char algstr[10];

                char qtmstr[10];

                char numofp[10];

                sprintf(algstr, "%d", algorithm);

                sprintf(qtmstr, "%d", qtm);

                sprintf(numofp, "%d", numofProcesses);

                execl("./scheduler.out", "scheduler.out", algstr, qtmstr, numofp, NULL);

            }

        }

        else



        {

            if (i == 0)

            {

                clk_id = pidd;

            }

            else

            {

                scheduler_id = pidd;

            }

        }

    }

}



int main(int argc, char *argv[])

{

    signal(SIGINT, clearResources);

    // TODO Initialization

    char* filePath = argv[1];

    algorithm=atoi(argv[2]);

    qtm=atoi(argv[3]);

    readFile(filePath); // 1. Read the input files. (done)



    //choose_scheduling_algo();

    create_clock_and_scheduler(); // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any. (done)

    // create_scheduler();                                 // 3. Initiate and create the scheduler and clock processes.

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

    msgqid = msgget(key, 0666 | IPC_CREAT); // this message queue will comunicate process generator with scheduler



    int it = 0;

    kill(scheduler_id,SIGCONT);

    

    while (it < numofProcesses)

    {

        int clknow = getClk();

        if (it < numofProcesses && ((processes + it)->arrival_time == clknow))

        {

            clknow = getClk();

            int val = msgsnd(msgqid, &(processes[it]), sizeof(struct Process), 0);

            if (val == -1)

            {

                printf("Error while sending a process to the scheduler\n");

            }

            else

            {

                // printf("sent a process to scheduler at %d \n", clknow);

            }

            it++;

        }

    }



    int status;

    waitpid(scheduler_id, &status, 0);

    printf("Child process finished with status %d\n", status);

    clearResources(0);

    destroyClk(true);

}

// processes.txt