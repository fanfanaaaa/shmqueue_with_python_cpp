#include <string.h>
#include <stdio.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include "shmmqueueWapper.h"

CMessageQueue* CreateInstance(key_t shmkey, size_t queuesize, eQueueModel queueModule){
    if(queuesize <= 0)
        return NULL;
    queuesize = IsPowerOfTwo(queuesize) ? queuesize : RoundupPowofTwo(queuesize);
    if(queuesize <= 0) {
        return NULL;
    }
    enShmModule shmModule;
    int shmId = 0;
    BYTE * tmpMem = CreateShareMem(shmkey, queuesize + sizeof(stMemTrunk), &shmModule, &shmId);
    if (tmpMem == NULL) 
        return NULL;
    
    CMessageQueue *messageQueue = (CMessageQueue *)malloc(sizeof(CMessageQueue));
    if (messageQueue == NULL) {
        // Handle memory allocation failure
        free(tmpMem);
        return NULL;
    }
    // Initialize the CMessageQueue instance
    messageQueue->m_stMemTrunk = (stMemTrunk *)tmpMem;
    InitLock(messageQueue);

    messageQueue->m_pQueueAddr = tmpMem + sizeof(stMemTrunk);
    messageQueue->m_pShm = tmpMem;

    messageQueue->m_stMemTrunk->m_iBegin = 0;
    messageQueue->m_stMemTrunk->m_iEnd = 0;
    messageQueue->m_stMemTrunk->m_iShmKey = shmkey;
    messageQueue->m_stMemTrunk->m_iSize = queuesize;
    messageQueue->m_stMemTrunk->m_iShmId = shmId;
    messageQueue->m_stMemTrunk->m_eQueueModule = queueModule;
    PrintTrunk(messageQueue);
    return messageQueue;
}
CMessageQueue* GetInstance(key_t shmkey, size_t queuesize, eQueueModel queueModule){
    if(queuesize <= 0)
        return NULL;

    queuesize = IsPowerOfTwo(queuesize) ? queuesize : RoundupPowofTwo(queuesize);
    if(queuesize <= 0) {
        return NULL;
    }
    enShmModule shmModule;
    int shmId = 0;
    BYTE * tmpMem = CreateShareMem(shmkey, queuesize + sizeof(stMemTrunk), &shmModule, &shmId);
    if (tmpMem == NULL) 
        return NULL;
    CMessageQueue *messageQueue = (CMessageQueue *)malloc(sizeof(CMessageQueue));
    if (messageQueue == NULL) {
        free(tmpMem);
        return NULL;
    }
    // Initialize the CMessageQueue instance
    messageQueue->m_stMemTrunk = (stMemTrunk *)tmpMem;
    InitLock(messageQueue);

    messageQueue->m_pQueueAddr = tmpMem + sizeof(stMemTrunk);
    messageQueue->m_pShm = tmpMem;

    PrintTrunk(messageQueue);
    return messageQueue;
    
}
void DestroyInstance(CMessageQueue* messageQueue){
    if (messageQueue == NULL) {
        return;
    }
    if (messageQueue->m_stMemTrunk != NULL) {
        DestroyShareMem(messageQueue->m_pShm, messageQueue->m_stMemTrunk->m_iShmKey);
    }
    // Assuming DestroyShareMem releases the memory and no explicit destructor is needed
    if (messageQueue->m_pBeginLock != NULL) {
        // Assuming m_pBeginLock is a pointer to a lock structure and needs to be deallocated
        free(messageQueue->m_pBeginLock);
    }
    if (messageQueue->m_pEndLock != NULL) {
        // Assuming m_pEndLock is a pointer to a lock structure and needs to be deallocated
        free(messageQueue->m_pEndLock);
    }
    free(messageQueue);
}

int SendMessage(CMessageQueue* messageQueue, BYTE* message, size_t length){
    if (!message || length <= 0) {
        return (int) QUEUE_PARAM_ERROR;
    }
    CSafeShmWlock tmLock;
    //修改共享内存写锁
    if (IsEndLock(messageQueue) && messageQueue->m_pEndLock) {
        CSafeShmWlock_Init(&tmLock, messageQueue->m_pEndLock);
    }

    // 首先判断是否队列已满
    size_t size = GetFreeSize(messageQueue);
    if (size <= 0) {
        printf("The queue is full enough to accommodate the additional data\n");
        return (int) QUEUE_NO_SPACE;
    }

    //空间不足
    if ((length + sizeof(MESS_SIZE_TYPE)) > size) {
        printf("The queue size is not large enough to accommodate the data size\n");
        return (int) QUEUE_NO_SPACE;
    }

    MESS_SIZE_TYPE usInLength = length;
    BYTE *pTempDst = messageQueue->m_pQueueAddr;
    BYTE *pTempSrc = (BYTE *) (&usInLength);

    //写入的时候我们在数据头插上数据的长度，方便准确取数据,每次写入一个字节可能会分散在队列的头和尾
    size_t tmpEnd = messageQueue->m_stMemTrunk->m_iEnd;
    for (MESS_SIZE_TYPE i = 0; i < sizeof(usInLength); i++) {
        pTempDst[tmpEnd] = pTempSrc[i];  // 拷贝 Code 的长度
        tmpEnd = (tmpEnd + 1) & (messageQueue->m_stMemTrunk->m_iSize - 1);  // % 用于防止 Code 结尾的 idx 超出 codequeue
    }
    size_t tmpLen = SHM_MIN(usInLength, messageQueue->m_stMemTrunk->m_iSize - tmpEnd);
    memcpy((void *) (&pTempDst[tmpEnd]), (const void *) message, (size_t) tmpLen);
    size_t tmpLastLen = length - tmpLen;
    if(tmpLastLen > 0)
    {
        /* then put the rest (if any) at the beginning of the buffer */
        memcpy(&pTempDst[0], message + tmpLen, tmpLastLen);
    }

    /*
    * Ensure that we add the bytes to the kfifo -before-
    * we update the fifo->in index.
    * 数据写入完成修改m_iEnd，保证读端不会读到写入一半的数据
    */
    __WRITE_BARRIER__;
    messageQueue->m_stMemTrunk->m_iEnd = (tmpEnd + usInLength) & (messageQueue->m_stMemTrunk->m_iSize -1);
    //printf("sendMessage successfully,  messageQueue->m_stMemTrunk->m_iEnd = %d\n",  messageQueue->m_stMemTrunk->m_iEnd);
    return (int) QUEUE_OK;
}
int GetMessage(CMessageQueue* messageQueue, BYTE* pOutCode){
    if (!pOutCode) {
        return (int) QUEUE_PARAM_ERROR;
    }

    CSafeShmWlock tmLock;
    //修改共享内存写锁
    if (IsBeginLock(messageQueue) && messageQueue->m_pBeginLock) {
        CSafeShmWlock_Init(&tmLock, messageQueue->m_pBeginLock);
    }

    int nTempMaxLength = GetDataSize(messageQueue);
    if (nTempMaxLength <= 0) {
        return (int) QUEUE_NO_MESSAGE;
    }

    BYTE *pTempSrc = messageQueue->m_pQueueAddr;
    // 如果数据的最大长度不到sizeof(MESS_SIZE_TYPE)（存入数据时在数据头插入了数据的长度,长度本身）
    if (nTempMaxLength <= (int) sizeof(MESS_SIZE_TYPE)) {
        printf("[%s:%d] ReadHeadMessage data len illegal,nTempMaxLength %d \n", __FILE__, __LINE__, nTempMaxLength);
        PrintTrunk(messageQueue);
        messageQueue->m_stMemTrunk->m_iBegin = messageQueue->m_stMemTrunk->m_iEnd;
        return (int) QUEUE_DATA_SEQUENCE_ERROR;
    }

    MESS_SIZE_TYPE usOutLength;
    BYTE *pTempDst = (BYTE *) &usOutLength;   // 数据拷贝的目的地址
    unsigned int tmpBegin = messageQueue->m_stMemTrunk->m_iBegin;
    //取出数据的长度
    for (MESS_SIZE_TYPE i = 0; i < sizeof(MESS_SIZE_TYPE); i++) {
        pTempDst[i] = pTempSrc[tmpBegin];
        tmpBegin = (tmpBegin + 1)  & (messageQueue->m_stMemTrunk->m_iSize -1);
    }

    // 将数据长度回传
    //取出的数据的长度实际有的数据长度，非法
    if (usOutLength > (int) (nTempMaxLength - sizeof(MESS_SIZE_TYPE)) || usOutLength < 0) {
        printf("[%s:%d] ReadHeadMessage usOutLength illegal,usOutLength: %d,nTempMaxLength %d \n",
               __FILE__, __LINE__, usOutLength, nTempMaxLength);
        PrintTrunk(messageQueue);
        messageQueue->m_stMemTrunk->m_iBegin = messageQueue->m_stMemTrunk->m_iEnd;
        return (int) QUEUE_DATA_SEQUENCE_ERROR;
    }

    pTempDst = &pOutCode[0];  // 设置接收 Code 的地址
    unsigned int tmpLen = SHM_MIN(usOutLength, messageQueue->m_stMemTrunk->m_iSize  - tmpBegin);
    memcpy(&pTempDst[0],&pTempSrc[tmpBegin], tmpLen);
    unsigned int tmpLast = usOutLength - tmpLen;
    if(tmpLast > 0)
    {
        memcpy(&pTempDst[tmpLen], pTempSrc, tmpLast);
    }

    __WRITE_BARRIER__;
    messageQueue->m_stMemTrunk->m_iBegin = (tmpBegin + usOutLength) & (messageQueue->m_stMemTrunk->m_iSize -1);    
    return usOutLength;
}
int ReadHeadMessage(CMessageQueue* messageQueue, BYTE* pOutCode){
    if (!pOutCode) {
        return (int)QUEUE_PARAM_ERROR;
    }
    CSafeShmRlock tmLock;

    // 修改共享内存写锁
    if (IsBeginLock(messageQueue) && messageQueue->m_pBeginLock) {
        CSafeShmWlock_Init(&tmLock, messageQueue->m_pBeginLock);
    }

    int nTempMaxLength = GetDataSize(messageQueue);

    if (nTempMaxLength <= 0) {
        return (int)QUEUE_NO_MESSAGE;
    }

    BYTE *pTempSrc = messageQueue->m_pQueueAddr;

    // 如果数据的最大长度不到sizeof(MESS_SIZE_TYPE)（存入数据时在数据头插入了数据的长度,长度本身）
    if (nTempMaxLength <= (int)sizeof(MESS_SIZE_TYPE)) {
        printf("[%s:%d] ReadHeadMessage data len illegal,nTempMaxLength %d \n", __FILE__, __LINE__, nTempMaxLength);
        PrintTrunk(messageQueue);
        return (int)QUEUE_DATA_SEQUENCE_ERROR;
    }

    MESS_SIZE_TYPE usOutLength;
    BYTE *pTempDst = (BYTE *)&usOutLength; // 数据拷贝的目的地址
    unsigned int tmpBegin = messageQueue->m_stMemTrunk->m_iBegin;

    // 取出数据的长度
    for (MESS_SIZE_TYPE i = 0; i < sizeof(MESS_SIZE_TYPE); i++) {
        pTempDst[i] = pTempSrc[tmpBegin];
        tmpBegin = (tmpBegin + 1) & (messageQueue->m_stMemTrunk->m_iSize - 1);
    }

    // 将数据长度回传
    // 取出的数据的长度实际有的数据长度，非法
    if (usOutLength > (int)(nTempMaxLength - sizeof(MESS_SIZE_TYPE)) || usOutLength < 0) {
        printf("[%s:%d] ReadHeadMessage usOutLength illegal,usOutLength: %d,nTempMaxLength %d \n",
               __FILE__, __LINE__, usOutLength, nTempMaxLength);
        PrintTrunk(messageQueue);
        return (int)QUEUE_DATA_SEQUENCE_ERROR;
    }

    pTempDst = &pOutCode[0]; // 设置接收 Code 的地址

    unsigned int tmpIndex = tmpBegin & (messageQueue->m_stMemTrunk->m_iSize - 1);
    unsigned int tmpLen = SHM_MIN(usOutLength, messageQueue->m_stMemTrunk->m_iSize - tmpIndex);
    memcpy(pTempDst, pTempSrc + tmpBegin, tmpLen);

    unsigned int tmpLast = usOutLength - tmpLen;

    if (tmpLast > 0) {
        memcpy(pTempDst + tmpLen, pTempSrc, tmpLast);
    }

    return usOutLength;
}
int DeleteHeadMessage(CMessageQueue *messageQueue) {
    CSafeShmWlock tmLock;

    // 修改共享内存写锁
    if (IsBeginLock(messageQueue) && messageQueue->m_pBeginLock) {
        CSafeShmWlock_Init(&tmLock, messageQueue->m_pBeginLock);
    }

    int nTempMaxLength = GetDataSize(messageQueue);
    if (nTempMaxLength <= 0) {
        return (int)QUEUE_NO_MESSAGE;
    }

    BYTE *pTempSrc = messageQueue->m_pQueueAddr;

    // 如果数据的最大长度不到sizeof(MESS_SIZE_TYPE)（存入数据时在数据头插入了数据的长度,长度本身）
    if (nTempMaxLength <= (int)sizeof(MESS_SIZE_TYPE)) {
        printf("[%s:%d] ReadHeadMessage data len illegal,nTempMaxLength %d \n", __FILE__, __LINE__, nTempMaxLength);
        PrintTrunk(messageQueue);
        messageQueue->m_stMemTrunk->m_iBegin = messageQueue->m_stMemTrunk->m_iEnd;
        return (int)QUEUE_DATA_SEQUENCE_ERROR;
    }

    MESS_SIZE_TYPE usOutLength;
    BYTE *pTempDst = (BYTE *)&usOutLength; // 数据拷贝的目的地址
    unsigned int tmpBegin = messageQueue->m_stMemTrunk->m_iBegin;

    // 取出数据的长度
    for (MESS_SIZE_TYPE i = 0; i < sizeof(MESS_SIZE_TYPE); i++) {
        pTempDst[i] = pTempSrc[tmpBegin];
        tmpBegin = (tmpBegin + 1) & (messageQueue->m_stMemTrunk->m_iSize - 1);
    }

    // 将数据长度回传
    // 取出的数据的长度实际有的数据长度，非法
    if (usOutLength > (int)(nTempMaxLength - sizeof(MESS_SIZE_TYPE)) || usOutLength < 0) {
        printf("[%s:%d] ReadHeadMessage usOutLength illegal,usOutLength: %d,nTempMaxLength %d \n",
               __FILE__, __LINE__, usOutLength, nTempMaxLength);
        PrintTrunk(messageQueue);
        messageQueue->m_stMemTrunk->m_iBegin = messageQueue->m_stMemTrunk->m_iEnd;
        return (int)QUEUE_DATA_SEQUENCE_ERROR;
    }

    messageQueue->m_stMemTrunk->m_iBegin = (tmpBegin + usOutLength) & (messageQueue->m_stMemTrunk->m_iSize - 1);
    return usOutLength;
}

void PrintTrunk(CMessageQueue* messageQueue){
    printf("Mem trunk address 0x%p,shmkey %d ,shmid %d, size %zu, begin %zu, end %zu, queue module %d \n",
            messageQueue->m_stMemTrunk,
            messageQueue->m_stMemTrunk->m_iShmKey,
            messageQueue->m_stMemTrunk->m_iShmId,
            messageQueue->m_stMemTrunk->m_iSize,
            messageQueue->m_stMemTrunk->m_iBegin,
            messageQueue->m_stMemTrunk->m_iEnd,
            messageQueue->m_stMemTrunk->m_eQueueModule);
}

size_t GetFreeSize(CMessageQueue* messageQueue){
    //长度应该减去预留部分长度8，保证首尾不会相接
    return GetQueueLength(messageQueue) - GetDataSize(messageQueue) - EXTRA_BYTE;
}
size_t GetDataSize(CMessageQueue* messageQueue){
     //第一次写数据前
    if (messageQueue->m_stMemTrunk->m_iBegin == messageQueue->m_stMemTrunk->m_iEnd) {
        return 0;
    }
        //数据在两头
    else if (messageQueue->m_stMemTrunk->m_iBegin > messageQueue->m_stMemTrunk->m_iEnd) {

        return  (size_t)(messageQueue->m_stMemTrunk->m_iEnd + messageQueue->m_stMemTrunk->m_iSize  - messageQueue->m_stMemTrunk->m_iBegin);
    }
    else   //数据在中间
    {
        return messageQueue->m_stMemTrunk->m_iEnd - messageQueue->m_stMemTrunk->m_iBegin;
    }
}

size_t GetQueueLength(CMessageQueue* messageQueue){
    return (size_t) messageQueue->m_stMemTrunk->m_iSize;
}

void InitLock(CMessageQueue* messageQueue){
    if (IsBeginLock(messageQueue)) {
        CShmRWlock_Init(messageQueue->m_pBeginLock, (key_t)(messageQueue->m_stMemTrunk->m_iShmKey + 1));
    }
    if (IsEndLock(messageQueue)) {
        CShmRWlock_Init(messageQueue->m_pEndLock,(key_t)(messageQueue->m_stMemTrunk->m_iShmKey + 2));
    }
}

int IsBeginLock(CMessageQueue* messageQueue){
     return (messageQueue->m_stMemTrunk->m_eQueueModule == MUL_READ_MUL_WRITE ||
        messageQueue->m_stMemTrunk->m_eQueueModule == MUL_READ_ONE_WRITE);
}

int IsEndLock(CMessageQueue* messageQueue){
    return (messageQueue->m_stMemTrunk->m_eQueueModule == MUL_READ_MUL_WRITE ||
        messageQueue->m_stMemTrunk->m_eQueueModule == ONE_READ_MUL_WRITE);
}
BYTE *CreateShareMem(key_t iKey, long vSize, enShmModule *shmModule, int *shmId) {
    size_t iTempShmSize;

    if (iKey < 0) {
        printf("[%s:%d] CreateShareMem failed. [key %d]errno:%s \n", __FILE__, __LINE__, iKey, strerror(errno));
        exit(-1);
    }

    iTempShmSize = (size_t)vSize;
    printf("Try to malloc share memory of %ld bytes... \n", iTempShmSize);
    *shmId = shmget(iKey, iTempShmSize, IPC_CREAT | IPC_EXCL | 0666);
    if (*shmId < 0) {
        if (errno != EEXIST) {
            printf("[%s:%d] Alloc share memory failed, [iKey:%d] , size:%ld , error:%s \n",
                   __FILE__, __LINE__, iKey, iTempShmSize, strerror(errno));
            exit(-1);
        }

        printf("Same shm seg [key= %d] exist, now try to attach it... \n", iKey);
        *shmId = shmget(iKey, iTempShmSize, IPC_CREAT | 0666);
        if (*shmId < 0) {
            printf("Attach to share memory [key= %d,shmId %d] failed, maybe the size of share memory changed,%s .now try to touch it again \n",
                   iKey, *shmId, strerror(errno));
            //先获取之前的shmId
            *shmId = shmget(iKey, 0, 0666);
            if (*shmId < 0) {
                printf("[%s:%d] Fatal error, touch to shm [key= %d,shmId %d] failed, %s.\n", __FILE__, __LINE__, iKey, *shmId, strerror(errno));
                exit(-1);
            } else {
                //先删除之前的share memory
                printf("First remove the exist share memory [key= %d,shmId %d] \n", iKey, *shmId);
                if (shmctl(*shmId, IPC_RMID, NULL)) {
                    printf("[%s:%d] Remove share memory [key= %d,shmId %d] failed, %s \n", __FILE__, __LINE__, iKey, *shmId, strerror(errno));
                    exit(-1);
                }
                //重新创建
                *shmId = shmget(iKey, iTempShmSize, IPC_CREAT | IPC_EXCL | 0666);
                if (*shmId < 0) {
                    printf("[%s:%d] Fatal error, alloc share memory [key= %d,shmId %d] failed, %s \n",
                           __FILE__, __LINE__, iKey, *shmId, strerror(errno));
                    exit(-1);
                }
            }
        } else {
            *shmModule = SHM_RESUME;
            printf("Attach to share memory [key= %d,shmId %d] succeed.\n", iKey, *shmId);
        }
    } else {
        *shmModule = SHM_INIT;
    }

    printf("Successfully alloced share memory block,[key= %d,shmId %d] size = %ld \n", iKey, *shmId, iTempShmSize);
    BYTE *tpShm = (BYTE *)shmat(*shmId, NULL, 0);

    if ((void *)-1 == tpShm) {
        printf("[%s:%d] create share memory failed, shmat failed, [key= %d,shmId %d], error = %s. \n",
               __FILE__, __LINE__, iKey, *shmId, strerror(errno));
        exit(0);
    }

    return tpShm;
}

int DestroyShareMem(const void* shmaddr, key_t iKey){
    int iShmID;

    if (iKey < 0) {
        printf("[%s:%d] Error in ftok, %s. \n", __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    printf("Touch to share memory [key = %d]... \n", iKey);
    iShmID = shmget(iKey, 0, 0666);
    if (iShmID < 0) {
        printf("[%s:%d] Error, touch to shm [key= %d,shmId %d] failed, %s \n", __FILE__, __LINE__, iKey, iShmID, strerror(errno));
        return -1;
    }
    else {
        printf("Now disconnect the exist share memory [key= %d,shmId %d] \n",  iKey, iShmID);
        if(shmdt(shmaddr)){
            printf("[%s:%d] Disconnect share memory [key= %d,shmId %d] failed, %s \n", __FILE__, __LINE__,iKey, iShmID,strerror(errno));
        } else{
            printf("Disconnect the exist share memory [key= %d,shmId %d] succeed \n", iKey, iShmID);
        }
        printf("Now remove the exist share memory [key= %d,shmId %d] \n", iKey, iShmID);
        if (shmctl(iShmID, IPC_RMID, NULL)) {
            printf("[%s:%d] Remove share memory [key= %d,shmId %d] failed, %s \n", __FILE__, __LINE__, iKey, iShmID,strerror(errno));
            return -1;
        } else{
            printf("Remove shared memory [key= %d,shmId %d] succeed. \n", iShmID, iKey);
        }
    }
    return 0;
}
int IsPowerOfTwo(size_t size){
    if(size < 1)
        return -1;//2的次幂一定大于0
    return ((size & (size -1)) == 0);
}
int Fls(size_t size){
    int position;
    int i;
    if(0 != size){
        for (i = (size >> 1), position = 0; i != 0; ++position)
            i >>= 1;
    }
    else{
        position = -1;
    }
    return position + 1;
}
size_t RoundupPowofTwo(size_t size){
    return 1UL << Fls(size - 1);
}