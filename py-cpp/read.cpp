#include <iostream>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <list>
#include "../shmmqueue.h"
#include <sys/wait.h> // for waitpid()  
#include <chrono>

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

size_t read(CMessageQueue *writeQueue)
{
    size_t size = 2 * 1024 * 1024 *1024ULL;
    BYTE* arr = new BYTE[size];
    size_t len = writeQueue->GetMessage(arr);
    if (len > 0) {
        // printf("[");
        // for (size_t i = 0; i < len; i++)
        // {
        //     printf("%d, ", arr[i]);
        // }
        // printf("]\n");
        printf("read success, len %zu\n", len);
        return len;
    }
    else {
        if (len != (int) eQueueErrorCode::QUEUE_NO_MESSAGE) {
            printf("Read failed ret1 = %d\n", len);
            writeQueue->PrintTrunk();
            exit(-1);
        }
        return -1;
    }
}
int main()  
{  
    size_t queuesize = 4 * 1024 * 1024 * 1024ULL;
    CMessageQueue *messQueue = CMessageQueue::GetInstance(12320,  queuesize, eQueueModel::ONE_READ_ONE_WRITE);  
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t len = read(messQueue); 
    printf("len %zu\n", len);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // 计算接收速率
    double receive_rate = static_cast<double>(len) / (duration.count() / 1000.0); 
    printf("Receive Rate: %f bytes per second\n", receive_rate);
    messQueue->PrintTrunk();

    //std::cout << "Receive Rate: " << receive_rate << " bytes per second" << std::endl;
}