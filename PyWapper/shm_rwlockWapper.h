#ifndef SAFE_SHM_WLOCK_H
#define SAFE_SHM_WLOCK_H
#include <sys/sem.h>
#include <sys/types.h>

typedef struct {
    int m_iSemID;
    key_t m_iSemKey;
} CShmRWlock;

void CShmRWlock_Init(CShmRWlock* rwlock, key_t iKey);
int CShmRWlock_Rlock(const CShmRWlock* rwlock);
int CShmRWlock_UnRlock(const CShmRWlock* rwlock);
int CShmRWlock_TryRlock(const CShmRWlock* rwlock);
int CShmRWlock_Wlock(const CShmRWlock* rwlock);
int CShmRWlock_UnWlock(const CShmRWlock* rwlock);
int CShmRWlock_TryWlock(const CShmRWlock* rwlock);
int CShmRWlock_Lock(const CShmRWlock* rwlock);
int CShmRWlock_Unlock(const CShmRWlock* rwlock);
int CShmRWlock_Trylock(const CShmRWlock* rwlock);
key_t CShmRWlock_Getkey(const CShmRWlock* rwlock);
int CShmRWlock_Getid(const CShmRWlock* rwlock);

typedef struct {
    CShmRWlock* m_pLock;
} CSafeShmRlock;

void CSafeShmRlock_Init(CSafeShmRlock* safe_rlock, CShmRWlock* pLock);
void CSafeShmRlock_Unlock(CSafeShmRlock* safe_rlock);


typedef struct {
    CShmRWlock* m_pLock;
} CSafeShmWlock;

void CSafeShmWlock_Init(CSafeShmWlock* safe_wlock, CShmRWlock* pLock);
void CSafeShmWlock_Unlock(CSafeShmWlock* safe_wlock);

#endif  // SHMQUEUE_SHM_RWLOCK_H
