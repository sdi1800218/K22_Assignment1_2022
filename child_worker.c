#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "common.h"

#define SUCCESS         0
#define ERROR_REQUEST   -2
#define ERROR_CHILD     -1

/* Perform the request */
void make_request(int64_t childNum, int64_t X, struct shared_use_st *shared_stuff) {

    /* Send Request */
    shared_stuff->request_line = rand() % X + 1;

    /* Notify finished request || Free SEM REQ */
    if (sem_post(&shared_stuff->semlock_request) == -1) {
        fprintf(stderr, "[CHILD %ld] Process failed to unlock semaphore\n", childNum);
    }
    //sem_state(&shared_stuff->semlock_request, &shared_stuff->semlock_reply, &shared_stuff->semlock_trans);

    /* Wait Reply */
    while (sem_wait(&shared_stuff->semlock_reply) == -1)
            if(errno != EINTR) {
                fprintf(stderr, "[CHILD %ld] Process failed to lock semaphore\n", childNum);
                return;
            }
    //fprintf(stderr, "[CLIENT] Reading Reply\n");
    fprintf(stderr, "[CHILD] I am child %ld, with PID: %d", childNum+1, getpid());
    fprintf(stderr, "\n\t I requested line %ld and got back \n\t~~> %s",
                    shared_stuff->request_line, shared_stuff->line);
    //sem_state(&shared_stuff->semlock_request, &shared_stuff->semlock_reply, &shared_stuff->semlock_trans);
}

/* Worker child */
void child_worker(int64_t childNum, int64_t totalLines, int64_t N) {
    /* Simple worker, returns a random int in shared mem as a request */

    sem_t *semlock_trans;
    double total = 0.0;

    /* SHM Stuff */
    int32_t shmid;
    struct shared_use_st *shared_stuff;
    void *shared_memory = (void *)0;

    srand(time(NULL) ^ getpid());

    /* Initialize shared mem SHM */
    if((shmid = shmget(SHM_KEY, sizeof(struct shared_use_st), 0666 | IPC_CREAT)) == -1) {
        perror("[CHILD] Failed to create shared memory segment for SHM");
        return;
    }

    /* Attach */
    if((shared_memory = shmat(shmid, (void *)0, 0)) == (void *)-1) {
        perror("[CHILD] Failed to attach memory segment SHM");
        return;
    }

    // map
    shared_stuff  = (struct shared_use_st *)shared_memory;
    semlock_trans = &shared_stuff->semlock_trans;


    /* N times critical */
    for (int64_t request = 0; request < N; ++request) {

        /************************** Entry section ****************************/
        while (sem_wait(semlock_trans) == -1) {
            if(errno != EINTR) {
                fprintf(stderr, "[CHILD %ld] Process failed to lock semaphore\n", childNum+1);
                return;
            }
            fprintf(stderr, "I am process %d, of child %ld and I am waiting for SEM TRANS\n", getpid(), childNum+1);
        }
        /************************** Critical Section *************************/
        fprintf(stderr, "[CHILD %ld] This is process %ld making request %ld\n", childNum+1, (long)getpid(), request+1);

        clock_t start = clock();

        make_request(childNum, totalLines, shared_stuff);

        clock_t end = clock();
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        total += time_spent;

        //fprintf(stderr, "\t\t[STATS] Took %lf sec\n", time_spent);

        /************************** Exit section *****************************/
        if (sem_post(semlock_trans) == -1) {
            fprintf(stderr, "[CHILD %ld] Process failed to unlock semaphore\n", childNum);
        }
    }

    fprintf(stderr, "[STATS] Median time of execution per request of Child %ld with PID: %d is : %lf sec\n", childNum+1, getpid(), (double)(total / N));

    /* Detach */
    if (shmdt(shared_memory) == -1) {
        fprintf(stderr, "[CHILD %ld] shmdt failed\n", childNum+1);
        return;
    }

    return;
}