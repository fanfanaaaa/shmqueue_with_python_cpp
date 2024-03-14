#ifndef messagequeue_h
#define messagequeue_h

#include "shm_rwlockWapper.h"

#define EXTRA_BYTE 8
#define MESS_SIZE_TYPE size_t

#define CACHELINE_SIZE 64
#define CACHELINE_ALIGN __attribute__((aligned(CACHELINE_SIZE)))

#define SHM_MIN(a, b) ((a) < (b) ? (a) : (b))

#define CACHE_LINE_SIZE 64
// 内存屏障
#define __MEM_BARRIER \
    asm volatile("" : : : "memory")

// 内存读屏障
#define __READ_BARRIER__ \
    asm volatile("" : : : "memory")

// 内存写屏障
#define __WRITE_BARRIER__ \
    asm volatile("" : : : "memory")

typedef unsigned char BYTE;

typedef enum {
    ONE_READ_ONE_WRITE,
    ONE_READ_MUL_WRITE,
    MUL_READ_ONE_WRITE,
    MUL_READ_MUL_WRITE
} eQueueModel;

typedef enum {
    QUEUE_OK = 0,
    QUEUE_PARAM_ERROR = -1,
    QUEUE_NO_SPACE = -2,
    QUEUE_NO_MESSAGE = -3,
    QUEUE_DATA_SEQUENCE_ERROR = -4
} eQueueErrorCode;

typedef enum {
    SHM_INIT,
    SHM_RESUME
} enShmModule;

typedef struct CACHELINE_ALIGN{
    volatile size_t m_iBegin;
    char __cache_padding1__[CACHE_LINE_SIZE];
    volatile size_t m_iEnd;
    char __cache_padding2__[CACHE_LINE_SIZE];
    int m_iShmKey;
    char __cache_padding3__[CACHE_LINE_SIZE];
    size_t m_iSize;
    char __cache_padding4__[CACHE_LINE_SIZE];
    int m_iShmId;
    char __cache_padding5__[CACHE_LINE_SIZE];
    eQueueModel m_eQueueModule;
} stMemTrunk;

typedef struct {
    stMemTrunk* m_stMemTrunk;
    CShmRWlock *m_pBeginLock;  //m_iBegin 锁
    CShmRWlock *m_pEndLock; //m_iEnd
    BYTE *m_pQueueAddr;
    void * m_pShm;
    // Add other members as needed
} CMessageQueue;

CMessageQueue* CreateInstance(key_t shmkey, size_t queuesize, eQueueModel queueModule);
CMessageQueue* GetInstance(key_t shmkey, size_t queuesize, eQueueModel queueModule);
void DestroyInstance(CMessageQueue* messageQueue);
int SendMessage(CMessageQueue* messageQueue, BYTE* message, size_t length);
int GetMessage(CMessageQueue* messageQueue, BYTE* pOutCode);
int ReadHeadMessage(CMessageQueue* messageQueue, BYTE* pOutCode);
int DeleteHeadMessage(CMessageQueue* messageQueue);
void PrintTrunk(CMessageQueue* messageQueue);

size_t GetFreeSize(CMessageQueue* messageQueue);
size_t GetDataSize(CMessageQueue* messageQueue);
size_t GetQueueLength(CMessageQueue* messageQueue);
void InitLock(CMessageQueue* messageQueue);
int IsBeginLock(CMessageQueue* messageQueue);
int IsEndLock(CMessageQueue* messageQueue);

BYTE* CreateShareMem(key_t iKey, long vSize, enShmModule* shmModule, int* shmId);
int DestroyShareMem(const void* shmaddr, key_t iKey);
int IsPowerOfTwo(size_t size);
int Fls(size_t size);
size_t RoundupPowofTwo(size_t size);

#endif  // messagequeue_h
