#include <iostream>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <list>
#include "../shmmqueue.h"
#include <sys/wait.h> // for waitpid()  

using namespace shmmqueue;


using namespace std;


void read_func(CMessageQueue *writeQueue, int threadId, const char *mes)
{
    BYTE arr1[100] = {0};
    int len1 = writeQueue->GetMessage(arr1);
    if (len1 > 0) {
        printf("[");
        for (size_t i = 0; i < len1; i++)
        {
            printf("%d, ", arr1[i]);
        }
        printf("]\n");
    }
    else {
        if (len1 != (int) eQueueErrorCode::QUEUE_NO_MESSAGE) {
            printf("Read failed ret1 = %d\n", len1);
            writeQueue->PrintTrunk();
            exit(-1);
        }
    }
    BYTE arr2[100] = {0};
    int len2 = writeQueue->GetMessage(arr2);
    if (len2 > 0) {
        printf("[");
        for (size_t i = 0; i < len1; i++)
        {
            printf("%d, ", arr2[i]);
        }
        printf("]\n");
    }
    else {
        if (len2 != (int) eQueueErrorCode::QUEUE_NO_MESSAGE) {
            printf("Read failed ret2 = %d\n", len2);
            writeQueue->PrintTrunk();
            exit(-1);
        }
    }

}
int main()  
{  
    CMessageQueue *messQueue = CMessageQueue::GetInstance(123,  2 * 80, eQueueModel::ONE_READ_ONE_WRITE);  
    messQueue->PrintTrunk();
    read_func(messQueue, 0, "Test_Read");  
}