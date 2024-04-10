#include "headers.h"

int msgqid,sigshmid;
int* sigshmaddr;
int main(int argc, char * argv[])
{
    initClk();
    
                        //TODO implement the scheduler :)
                       //upon termination release the clock resources.



                         key_t key = ftok("key", 'p');
        msgqid = msgget(key, 0666 | IPC_CREAT);// this message queue will comunicate process generator with scheduler

        printf("hi from scheduler\n");


    // destroyClk(true);
}
