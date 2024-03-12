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

std::atomic_int read_count;

std::atomic_int write_count;

int read_i = 0;

int write_i = 0;

atomic_bool done_flag;

#define SING_TEST_NUM 1000000
#define  THREAD_NUM 5
#define THREAD_SEND_NUM 100000

long long getCurrentTime()
{
    auto time_now = chrono::system_clock::now();
    auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(time_now.time_since_epoch());
    return duration_in_ms.count();
}

void write_func(CMessageQueue *writeQueue, int threadId, const char *mes)
{
    int a = 666;
    const string &data = to_string(a);
    int iRet = writeQueue->SendMessage((BYTE *) data.c_str(), data.length());
    printf("write data %d success\n", a);
    writeQueue->PrintTrunk();   
}
int main()  
{  
    CMessageQueue *messQueue = CMessageQueue::CreateInstance(SHAR_KEY_1, 10240, eQueueModel::ONE_READ_ONE_WRITE);  
    write_func(messQueue, 0, "Test_Write");  


}