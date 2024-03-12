
#include <sys/ipc.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>  // For malloc
#include <stdio.h>   // For printf
#include "shm_rwlockWapper.h"

union semun {
    int val;                /* value for SETVAL */
    struct semid_ds *buf;   /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;  /* array for GETALL, SETALL */
};

void CShmRWlock_Init(CShmRWlock* rwlock, key_t iKey) {
    int iSemID;
    union semun arg;
    unsigned short array[2] = {0, 0};
    if ((iSemID = semget(iKey, 2, IPC_CREAT | IPC_EXCL | 0666)) != -1) {
            arg.array = &array[0];
            if (semctl(iSemID, 0, SETALL, arg) == -1) {
                perror("semctl error\n");
            }
        }
        else {
            if (errno != EEXIST) {
                perror("sem has exist error");
            }
            if ((iSemID = semget(iKey, 2, 0666)) == -1) {
                perror("semget error\n");
            }
        }

    rwlock->m_iSemKey = iKey;
    rwlock->m_iSemID = iSemID;
}

int CShmRWlock_Rlock(const CShmRWlock* rwlock){
    /*包含两个信号量,第一个为写信号量，第二个为读信号量
     *获取读锁
     *等待写信号量（第一个）变为0：{0, 0, SEM_UNDO},并且把读信号量（第一个）加一：{1, 1, SEM_UNDO}
     **/
    struct sembuf sops[2] = {{0, 0, SEM_UNDO}, {1, 1, SEM_UNDO}};
    size_t nsops = 2;
    int ret = -1;

    do {
        ret = semop(rwlock->m_iSemID, &sops[0], nsops);
    }
    while ((ret == -1) && (errno == EINTR));

    return ret;
}

int CShmRWlock_UnRlock(const CShmRWlock* rwlock){
        /*包含两个信号量,第一个为写信号量，第二个为读信号量
     *解除读锁
     *把读信号量（第二个）减一：{1, -1, SEM_UNDO}
     **/
    struct sembuf sops[1] = {{1, -1, SEM_UNDO}};
    size_t nsops = 1;

    int ret = -1;

    do {
        ret = semop(rwlock->m_iSemID, &sops[0], nsops);

    }
    while ((ret == -1) && (errno == EINTR));

    return ret;
}
int CShmRWlock_TryRlock(const CShmRWlock* rwlock){
     /*包含两个信号量,第一个为写信号量，第二个为读信号量
     *获取读锁
     *尝试等待写信号量（第一个）变为0：{0, 0,SEM_UNDO | IPC_NOWAIT},
     *把读信号量（第一个）加一：{1, 1,SEM_UNDO | IPC_NOWAIT}
     **/
    struct sembuf sops[2] = {{0, 0, SEM_UNDO | IPC_NOWAIT}, {1, 1, SEM_UNDO | IPC_NOWAIT}};
    size_t nsops = 2;

    int iRet = semop(rwlock->m_iSemID, &sops[0], nsops);
    if (iRet == -1) {
        if (errno == EAGAIN) {
            //无法获得锁
            return -1;
        }
        else {
            perror("semop error\n");
        }
    }
    return 1;
}
int CShmRWlock_Wlock(const CShmRWlock* rwlock){
    /*包含两个信号量,第一个为写信号量，第二个为读信号量
     *获取写锁
     *尝试等待写信号量（第一个）变为0：{0, 0, SEM_UNDO},并且等待读信号量（第二个）变为0：{0, 0, SEM_UNDO}
     *把写信号量（第一个）加一：{0, 1, SEM_UNDO}
     **/
    struct sembuf sops[3] = {{0, 0, SEM_UNDO}, {1, 0, SEM_UNDO}, {0, 1, SEM_UNDO}};
    size_t nsops = 3;

    int ret = -1;

    do {
        ret = semop(rwlock->m_iSemID, &sops[0], nsops);

    }
    while ((ret == -1) && (errno == EINTR));

    return ret;
}
int CShmRWlock_UnWlock(const CShmRWlock* rwlock){
      /*包含两个信号量,第一个为写信号量，第二个为读信号量
     *解除写锁
     *把写信号量（第一个）减一：{0, -1, SEM_UNDO}
     **/
    struct sembuf sops[1] = {{0, -1, SEM_UNDO}};
    size_t nsops = 1;

    int ret = -1;

    do {
        ret = semop(rwlock->m_iSemID, &sops[0], nsops);

    }
    while ((ret == -1) && (errno == EINTR));

    return ret;
}

int CShmRWlock_TryWlock(const CShmRWlock* rwlock){
     /*包含两个信号量,第一个为写信号量，第二个为读信号量
     *尝试获取写锁
     *尝试等待写信号量（第一个）变为0：{0, 0, SEM_UNDO | IPC_NOWAIT},并且尝试等待读信号量（第二个）变为0：{0, 0, SEM_UNDO | IPC_NOWAIT}
     *把写信号量（第一个）加一：{0, 1, SEM_UNDO | IPC_NOWAIT}
     **/
    struct sembuf sops[3] = {{0, 0, SEM_UNDO | IPC_NOWAIT},
                             {1, 0, SEM_UNDO | IPC_NOWAIT},
                             {0, 1, SEM_UNDO | IPC_NOWAIT}};
    size_t nsops = 3;

    int iRet = semop(rwlock->m_iSemID, &sops[0], nsops);
    if (iRet == -1) {
        if (errno == EAGAIN) {
            //无法获得锁
            return -1;
        }
        else {
            perror("semop error\n");
        }
    }
    return 1;
}
int CShmRWlock_Lock(const CShmRWlock* rwlock){
    return CShmRWlock_Wlock(rwlock);
}

int CShmRWlock_Unlock(const CShmRWlock* rwlock){
    return CShmRWlock_UnWlock(rwlock);
}

int CShmRWlock_Trylock(const CShmRWlock* rwlock){
    return CShmRWlock_TryWlock(rwlock);
}

key_t CShmRWlock_Getkey(const CShmRWlock* rwlock){
    return rwlock->m_iSemKey;
}
int CShmRWlock_Getid(const CShmRWlock* rwlock){
    return rwlock->m_iSemID;
}

void CSafeShmRlock_Init(CSafeShmRlock* safe_rlock, CShmRWlock* pLock){
    safe_rlock->m_pLock = pLock;
    if (safe_rlock->m_pLock != NULL) {
        CShmRWlock_Rlock(safe_rlock->m_pLock);
    }
}

void CSafeShmRlock_Unlock(CSafeShmRlock* safe_rlock){
    if (safe_rlock != NULL && safe_rlock->m_pLock != NULL) {
        CShmRWlock_UnRlock(safe_rlock->m_pLock);
    }
}

void CSafeShmWlock_Init(CSafeShmWlock* safe_wlock, CShmRWlock* pLock){
    safe_wlock->m_pLock = pLock;
    if (safe_wlock->m_pLock != NULL) {
        CShmRWlock_Wlock(safe_wlock->m_pLock);
    }
}
void CSafeShmWlock_Unlock(CSafeShmWlock* safe_wlock){
    if (safe_wlock != NULL && safe_wlock->m_pLock != NULL) {
        CShmRWlock_UnWlock(safe_wlock->m_pLock);
        }
}