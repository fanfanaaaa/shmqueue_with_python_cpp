#include <iostream>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <list>
#include "../shmmqueue.h"
#include <sys/wait.h> // for waitpid()  

using namespace shmmqueue;

#define SHAR_KEY_1 100010
#define SHAR_KEY_2 100020

using namespace std;


void read_func(CMessageQueue *writeQueue, int threadId, const char *mes)
{
    BYTE data[100] = {0};
    int len = writeQueue->GetMessage(data);
    if (len > 0) {
        int i = atoi((const char *) data);
        printf("recv data is %d\n",i);
    }
    else {
        if (len != (int) eQueueErrorCode::QUEUE_NO_MESSAGE) {
            printf("Read failed ret = %d\n", len);
            writeQueue->PrintTrunk();
            exit(-1);
        }
    }
}
int main()  
{  
    CMessageQueue *messQueue = CMessageQueue::GetInstance(SHAR_KEY_1, 10240, eQueueModel::ONE_READ_ONE_WRITE);  
    messQueue->PrintTrunk();
    read_func(messQueue, 0, "Test_Read");  
}