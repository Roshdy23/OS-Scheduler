#include <stdio.h> //if you don't use scanf/printf change this include



#include <sys/types.h>



#include <sys/stat.h>



#include <sys/file.h>



#include <sys/ipc.h>



#include <sys/shm.h>



#include <sys/sem.h>



#include <sys/msg.h>



#include <sys/wait.h>



#include <stdlib.h>



#include <unistd.h>



#include <signal.h>



#include <math.h>



typedef short bool;



#define true 1



#define false 0



#define SHKEY 300



///==============================



// don't mess with this variable//



int *shmaddr; //



//===============================



int getClk()



{



    return *shmaddr;

}



int shmid;



/*



 * All process call this function at the beginning to establish communication between them and the clock module.



 * Again, remember that the clock is only emulation!



 */



void initClk()



{



    shmid = shmget(SHKEY, 4, 0444);



    while ((int)shmid == -1)



    {



        // Make sure that the clock exists



        printf("Wait! The clock not initialized yet!\n");



        sleep(1);



        shmid = shmget(SHKEY, 4, 0444);

    }



    shmaddr = (int *)shmat(shmid, (void *)0, 0);

}



/*



 * All process call this function at the end to release the communication



 * resources between them and the clock module.



 * Again, Remember that the clock is only emulation!



 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.



 *                      It terminates the whole system and releases resources.



 */



void destroyClk(bool terminateAll)



{



    shmdt(shmaddr);



    if (terminateAll)



    {



        shmctl(shmid, IPC_RMID, NULL);



        killpg(getpgrp(), SIGINT);

    }

}



struct block; // block to hold the free memory places



struct block



{



    int start;



    int end;



    struct block *next;

};



struct block *memory[11]; // because the max is 1024



enum ProcessState



{



    ready,



    running,



    blocked,



    terminated



};



struct Process



{



    int id;



    int pid;



    int arrival_time;



    int runtime;



    int priority;



    int finishtime;



    int starttime;



    int waitingTime;



    int remainingTime;



    int lastStopTime;



    int start;



    int size;



    enum ProcessState state;

};



struct ProcessNode

{



    struct Process *process;



    struct ProcessNode *next;

};



union Semun



{



    int val; /* Value for SETVAL */



    struct semid_ds *buf; /* Buffer for IPC_STAT, IPC_SET */



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



//-------------------------- memory -----------------------------//



void initmemory()



{



    // set the block of 1024 bytes



    memory[10] = (struct block *)malloc(sizeof(struct block));



    (*(memory[10])).start = 0;



    (*(memory[10])).end = 1023;



    (*(memory[10])).next = NULL;



    for (int i = 0; i < 10; i++)



    {



        memory[i] = NULL;

    }

}



int allocatememory(struct Process *p, int time)



{



    int k = ceil(log2(p->size));



    int suitableindex = k;



    if (memory[k]) // the size is suitable for the process, so allocate it



    {



        p->start = memory[k]->start; // take the first slot as it is sorted



        memory[k] = memory[k]->next;

    }



    else // the free size is greater than the process size



    {



        while (!memory[suitableindex])



        {



            suitableindex++;



            if (suitableindex > 10)



                return -1; // the required size is more than the max free size exist

        }



        p->start = memory[suitableindex]->start;



        int sizeofdeletedblock;



        sizeofdeletedblock = memory[suitableindex]->end - memory[suitableindex]->start + 1; // get the size of the removed slot



        suitableindex--;



        while (suitableindex - k != -1)



        {



            sizeofdeletedblock = sizeofdeletedblock / 2;



            memory[suitableindex] = (struct block *)malloc(sizeof(struct block));



            memory[suitableindex]->start = memory[suitableindex + 1]->start; // start of the new slot is the same as the index after it



            memory[suitableindex]->end = memory[suitableindex]->start + sizeofdeletedblock - 1;



            memory[suitableindex]->next = (struct block *)malloc(sizeof(struct block));



            memory[suitableindex]->next->start = memory[suitableindex]->start + sizeofdeletedblock;



            memory[suitableindex]->next->end = memory[suitableindex + 1]->end;



            memory[suitableindex]->next->next = NULL;



            memory[suitableindex + 1] = memory[suitableindex + 1]->next; // remove the slot



            suitableindex--;

        }



        memory[suitableindex + 1] = memory[suitableindex + 1]->next; // remove the slot assigned to the process

    }



    int allocatedsize = pow(2, k);



    return 1;



    // printf("At time %d allocated %d bytes for process %d from %d to %d\n",time,p->size,p->id,p->start,p->start + allocatedsize - 1);



    // printf("the allocated space:start and the end addresses are %d,%d\n", p->start, p->start + allocatedsize - 1); // you can let it here or print after allocation of a process

}



void freememory(int start, int size)



{



    int k = ceil(log2(size));



    int freedsize = pow(2, k);



    int buddy = start ^ freedsize;



    struct block *temp = memory[k];



    struct block *prev = NULL;



    while (temp)



    {



        if (buddy == temp->start)



        {



            if (!prev)



            {



                memory[k] = memory[k]->next;

            }



            else



            {



                prev = temp->next;

            }



            int min = start < buddy ? start : buddy; // get the minimum of the start and buddy to be the start of the new slot of double freedsize



            freememory(min, freedsize * 2);



            return;

        }



        prev = temp;



        temp = temp->next;

    }



    struct block *newslot = (struct block *)malloc(sizeof(struct block));



    newslot->start = start;



    newslot->end = start + freedsize - 1;



    newslot->next = NULL;



    temp = memory[k];



    if (!temp)



    {



        memory[k] = newslot;

    }



    else if (temp->start > start)



    {



        newslot->next = temp;



        memory[k] = newslot;

    }



    else



    {



        while (temp->next && temp->next->start < start)



        {



            temp = temp->next;

        }



        newslot->next = temp->next;



        temp->next = newslot;

    }



    // printf("At time %d freed %d bytes from process %d from %d to %d\n",time,p->size,p->id,start, start + freedsize - 1);



    // printf("the freed space: start and the end addresses are %d,%d\n", start, start + freedsize - 1); // after termination the process print this with start and end parameters of the processes

}



void printmemory(int time, int size, int id, int start, int end, int op)

{



    FILE *memorylog = fopen("memory.log", "a");



    if (memorylog == NULL)

    {



        printf("Error opening file.\n");



        return;

    }



    int k = ceil(log2(size));



    if (op == 1)

    {



        int allocatedsize = pow(2, k);



        fprintf(memorylog, "At time %d allocated %d bytes for process %d from %d to %d\n", time, size, id, start, start + allocatedsize - 1);

    }

    else

    {



        int freedsize = pow(2, k);



        fprintf(memorylog, "At time %d freed %d bytes from process %d from %d to %d\n", time, size, id, start, start + freedsize - 1);

    }



    fclose(memorylog);

}



struct processnode

{



    struct Process *pp;



    struct processnode *next;

};

struct RRqueue {

    struct processnode* front;

    struct processnode* rear;

};

struct processnode* newprocessnode(struct Process*p) {

    struct processnode *node = (struct processnode *)malloc(sizeof(struct processnode));

    node->pp = p;

    node->next = NULL;

    return node;

}

struct RRqueue* createRRqueue() {

    struct RRqueue* RRqueue = (struct RRqueue*)malloc(sizeof(struct RRqueue));

    RRqueue->front = RRqueue->rear = NULL;

    return RRqueue;

}

void putinRRlist(struct RRqueue* queue, struct Process*p) {

    struct processnode* newnode = newprocessnode(p);

    if (queue->rear == NULL) {

        queue->front = queue->rear = newnode;

        return;

    }

    queue->rear->next = newnode;

    queue->rear = newnode;

}

// struct processnode *RRHead = NULL;



// void putinRRlist(struct Process *p)

// {



//     struct processnode *newNode = (struct processnode *)malloc(sizeof(struct processnode));



//     newNode->pp = p;



//     newNode->next = NULL;



//     if (head == NULL || head->pp->size < p->size)

//     {



//         newNode->next = head;



//         head = newNode;

//     }

//     else

//     {



//         struct processnode *current = head;



//         while (current->next != NULL && current->next->pp->size > p->size)

//         {



//             current = current->next;

//         }



//         newNode->next = current->next;

//         current->next = newNode;

//     }

// }

struct Process *getfromRRlist(struct RRqueue* queue) {

    if (queue->front == NULL) {

        printf("Queue is empty\n");

        return NULL;

    }

    struct Process* data = queue->front->pp;

    struct processnode* temp = queue->front;

    queue->front = queue->front->next;

    if (queue->front == NULL) {

        queue->rear = NULL;

    }

    free(temp);

    return data;

}

// struct Process *getfromlist()

// {



//     if (head == NULL)

//     {



//         return NULL; // List is empty



//     }



//     struct Process *result = head->pp;



//     struct processnode *temp = head;



//     head = head->next;



//     temp->next=NULL;

//     free(temp);



//     return result;

// }



//-------------------------- RR headers ------------------------//



struct Queue



{



    struct ProcessNode *front;



    struct ProcessNode *rear;

};



void initializeQueue(struct Queue *q)



{



    q->front = NULL;



    q->rear = NULL;

}



void enqueue(struct Queue *q, struct Process *p)



{



    struct ProcessNode *newProcess;



    newProcess = (struct ProcessNode *)malloc(sizeof(struct ProcessNode));



    newProcess->process = p;



    newProcess->next = NULL;



    if (q->front == NULL)



    {



        // if empty queue



        q->front = newProcess;



        q->rear = newProcess;

    }



    else



    {



        (q->rear)->next = newProcess;



        q->rear = newProcess;



        (q->rear)->next = NULL;

    }

}



struct Process *dequeue(struct Queue *q)



{



    if (q->front != NULL)



    {



        struct ProcessNode *n = q->front;



        struct Process *p = (q->front)->process;



        if (q->front == q->rear)



        {



            q->front = NULL;



            q->rear = NULL;

        }



        else



        {



            q->front = (q->front)->next;



            n->next = NULL;

        }



        return p;

    }



    else



        return NULL;

}



void displayQ(struct Queue q)



{



    struct ProcessNode *it = q.front;



    if (!q.front)



        printf("the queue is empty");



    while (it != NULL && it != q.rear)



    {



        printf("processID:%d\n", (it->process)->id);



        it = it->next;

    }



    if (it != NULL)



        printf("processID:%d\n", (it->process)->id);



    return;

}



bool isEmpty(struct Queue q)



{



    if (q.front == NULL)



        return true;



    else



        return false;

}



// -----------------------------------------Priority Queue Data Structure ------------------------------



struct priority_Queue



{



    struct ProcessNode *front;

};



void initializePriorityQueue(struct priority_Queue *q)



{



    q->front = NULL;

}



// enqueue dequeue peek isempty display



void priority_enqueue(struct priority_Queue *q, struct Process *p, int op)



{



    struct ProcessNode *newProcess;



    newProcess = (struct ProcessNode *)malloc(sizeof(struct ProcessNode));



    newProcess->process = p;



    newProcess->next = NULL;



    if (op == 0)



    {



        if (q->front == NULL || q->front->process->priority > newProcess->process->priority)



        {



            newProcess->next = q->front;



            q->front = newProcess;

        }



        else



        {



            struct ProcessNode *temp = q->front;



            while (temp->next && temp->next->process->priority <= p->priority)



                temp = temp->next;



            newProcess->next = temp->next;



            temp->next = newProcess;

        }

    }



    if (op == 1)



    {



        if (q->front == NULL || q->front->process->remainingTime > newProcess->process->remainingTime)



        {



            newProcess->next = q->front;



            q->front = newProcess;

        }



        else



        {



            struct ProcessNode *temp = q->front;



            while (temp->next && temp->next->process->remainingTime <= p->remainingTime)



                temp = temp->next;



            newProcess->next = temp->next;



            temp->next = newProcess;

        }

    }

}



struct Process *priority_peek(struct priority_Queue *q)



{



    if (q->front)



        return q->front->process;



    else



        return NULL;

}



struct Process *priority_dequeue(struct priority_Queue *q)



{



    if (q->front)



    {



        struct Process *temp = q->front->process;



        struct ProcessNode *temp2 = q->front;



        q->front = q->front->next;



        temp2->next = NULL;



        return temp;

    }



    return NULL;

}



bool priority_isempty(struct priority_Queue *q)



{



    return (q->front == NULL);

}



void priority_display(struct priority_Queue *q)



{



    struct ProcessNode *it = q->front;



    while (it)



    {



        printf("process id = %d , arrival time = %d , priority = %d \n", it->process->priority, it->process->id, it->process->arrival_time);



        it = it->next;

    }

}



//------------------------------------------------------------------------

