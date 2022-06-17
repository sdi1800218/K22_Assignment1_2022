#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>

#define USE_SHARED_MEMORY

#define MAXPIDS 	100
#define MAXREQUESTS 1000
#define BUFSIZE 	100
#define TEN_MILLION 10000000L

#define SHM_KEY         (key_t)0x1337
#define SEM_REQUEST_KEY (key_t)0xBABE
#define SEM_REPLY_KEY   (key_t)0xCAFE
#define SEM_TRANS_KEY   (key_t)0xAAAA//X

/* Thy problem */
struct shared_use_st {
    int64_t request_line;
    char    line[BUFSIZE];
    /* 3 Semaphores to rule them all */
    sem_t   semlock_request;
    sem_t   semlock_reply;
    sem_t   semlock_trans;
};

/* Child definition */
void child_worker(int64_t, int64_t, int64_t);

void sem_state(sem_t *, sem_t *, sem_t *);
void wait_sem(sem_t *, int32_t);