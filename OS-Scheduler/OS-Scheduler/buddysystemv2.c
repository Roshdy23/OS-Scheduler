#include <stdio.h>
#include <stdlib.h>
#include <math.h>
struct block; // block to hold the free memory places

struct block
{
    int start;
    int end;
    struct block *next;
};

struct process
{
    int start;
    int size;
};
struct block *memory[11]; // because the max is 1024

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

void allocatememory(struct process *p)
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
                return; // the required size is more than the max free size exist
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
    printf("the allocated space:start and the end addresses are %d,%d\n", p->start, p->start + allocatedsize - 1); // you can let it here or print after allocation of a process
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
    printf("the freed space: start and the end addresses are %d,%d\n", start, start + freedsize - 1); // after termination the process print this with start and end parameters of the processes
}
int main()
{
    initmemory();
    struct process test1, test2, test3, test4, test5;
    test1.size = 200;
    test2.size = 100;
    test3.size = 100;
    test4.size = 200;
    test5.size = 50;
    allocatememory(&test1);
    allocatememory(&test2);
    allocatememory(&test3);
    allocatememory(&test4);
    freememory(test1.start,test1.size);
    freememory(test3.start,test3.size);
    allocatememory(&test5);
    allocatememory(&test1);
    freememory(test2.start,test2.size);
    allocatememory(&test2);
     freememory(test5.start,test5.size);
      freememory(test1.start,test1.size);
      freememory(test2.start,test2.size); 
      freememory(test4.start,test4.size);
    return (0);
}