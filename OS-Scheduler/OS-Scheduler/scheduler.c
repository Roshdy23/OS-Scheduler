#include "headers.h"

void RR(int quantum);
int quantum ;
int terminatedProcessesNum = 0;
int numofProcesses;
int algorithm;
struct Process RunningProcess;

int main(int argc, char *argv[])
{
    initClk();
    printf("%d", argc);

    key_t key = ftok("key", 'p');         //message queue to recieve arrived processes
    int msgqid = msgget(key, 0666 | IPC_CREAT);
    if (msgqid == -1)
    {
        perror("Error in creating message queue");
        return -1;
    }

    algorithm = atoi(argv[1]);       //initializing variables
    quantum = atoi(argv[2]);
    numofProcesses = atoi(argv[3]);

    printf("%d ,%d ,%d ", algorithm, quantum, numofProcesses);

    switch (algorithm)
    {
    case 2:
        RR(quantum);
        break;
    
    default:
        break;
    }
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources.

    msgctl(msgqid, IPC_RMID, (struct msqid_ds *)0);

    destroyClk(true);
}
        //------------------------RR algorithm-----------------------------------//
void RR(int quantum){
    printf("executing RR algorithm\n");
    struct CircularQueue readyQueue;               //queue to store arrived processes
    initializeCircularQueue(&readyQueue);
    int remainQuantum;
    int currentTime=-1; 
    while(1){
        if (terminatedProcessesNum == numofProcesses)         //if all processes are terminated
            break;
        
        if(RunningProcess.remainingTime==0||remainQuantum==0){//switch processes
            if(RunningProcess.remainingTime==0){
                RunningProcess.state=terminated;
                terminatedProcessesNum++;
            }
            else{
                RunningProcess.state=ready;                   //change state to ready 
                enqueueInCQ(&readyQueue,RunningProcess);      
            }
            if(!isEmpty(readyQueue)){             
                remainQuantum=quantum;
                RunningProcess=dequeueOFCQ(&readyQueue);
                RunningProcess.state=running;
            }
        }
        if (RunningProcess.remainingTime && remainQuantum>0){
            RunningProcess.remainingTime-=1;
            remainQuantum-=1;
        }
        while(currentTime!=getClk());                       //waiting till the next clk 
        currentTime = getClk();                             //update time
    }

}
