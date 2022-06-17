/* userland.c: Spawns a K number of children (with fork()) which execute N number of requests; N and K as arguments.
                The child processes act as Clients and this program as the Server for the mentioned requests.
                Requests are numbers which specify a line in the file given as input. The Server's responses are these lines.
    This code is based on the 2nd Semaphore's Lab Posix example #2
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>

#include "common.h"

#define USE_SHARED_MEMORY

/* Given a requested line from the input file, it returns that line */
void getFileLine(FILE *fp, int64_t lineToGet, char *buffer) {
    int64_t count = 0;
    int8_t len = 100;

    //fprintf(stderr, "Getting line %ld from file\n", lineToGet);

    rewind(fp);

    /* Open thy file input */
     while (fgets(buffer, len, fp)) {
        //printf("%ld", count);
        if (count == lineToGet) {
            //fprintf(stderr, "GOT IT\n");
            return;
        }
        else
            ++count;
    }
}

/* Counts the total of lines in the file */
int64_t countLines(FILE *fp) {
    char ch;
    int64_t linesCount = 0;

    while( (ch = fgetc(fp)) != EOF) {
          if(ch == '\n')
             ++linesCount;
   }

   return linesCount;
}

/* Helper function to print los tres semaphores state */
void sem_state(sem_t *semlock_request, sem_t *semlock_reply, sem_t *semlock_trans) {
    int valueRequest, valueReply, valueTrans;
    sem_getvalue(semlock_request, &valueRequest);
    sem_getvalue(semlock_reply, &valueReply);
    sem_getvalue(semlock_trans, &valueTrans);
    fprintf(stderr, "Sem state: %d, %d, %d\n", valueRequest, valueReply, valueTrans);
}

/* Helper function to wait for a semaphore to be locked/unlocked */
void wait_sem(sem_t *mafor, int32_t target) {
    int32_t curr;
    do {
        sem_getvalue(mafor, &curr);
    } while (curr != target);
}

/* Parent/consumer */
int main(int argc, char *argv[]) {
    int64_t K, N;
    int64_t filesLines;

    FILE *fp;

    pid_t pids[MAXPIDS];

    /* SHM stuff */
    int32_t shmid;
    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;

    /* Assert correct number of cmd arguments */
    if (argc != 4){
        fprintf(stderr, "Usage: %s <file> <children> <requests>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Open file */
    fp = fopen(argv[1], "r");
    if(fp == NULL){
        fprintf(stderr, "[PARENT] Unable to open File = %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* Assert K less than Max and positive number */
    K = atoi(argv[2]);
    if (K > MAXPIDS) {
        fprintf(stderr, "K: %ld is too big\n", K);
        fprintf(stderr, "Try a value below: %d\n", MAXPIDS);
        exit(EXIT_FAILURE);
    }
    else if (K <= 0) {
        fprintf(stderr, "K: %ld value is invalid\n", K);
        exit(EXIT_FAILURE);
    }

    /* Assert N less than Max and positive number */
    N = atoi(argv[3]);
    if (N > MAXREQUESTS) {
        fprintf(stderr, "N: %ld is too big\n", N);
        fprintf(stderr, "Try a value below: %d\n", MAXREQUESTS);
        exit(EXIT_FAILURE);
    }
    else if (N < 0) {
        fprintf(stderr, "N: %ld value is invalid\n", N);
        exit(EXIT_FAILURE);
    }

    /* Get count X */
    filesLines = countLines(fp);
    //fprintf(stderr, "File has %ld lines\n", filesLines);

    /* Initialize shared mem SHM */
    if((shmid = shmget(SHM_KEY, sizeof(struct shared_use_st), 0666 | IPC_CREAT)) == -1) {
        perror("[PARENT] Failed to create shared memory segment for SHM");
        exit(EXIT_FAILURE);
    }

    /* Attach */
    if((shared_memory = shmat(shmid, (void *)0, 0)) == (void *)-1) {
        perror("[PARENT] Failed to attach memory segment SHM");
        exit(EXIT_FAILURE);
    }

    /* Init the shared mem data */
    shared_stuff = (struct shared_use_st *)shared_memory;
    shared_stuff->request_line = -1;
    memset(shared_stuff->line, 'A', 100);

    /* Initliaze the semaphore lock shared between all processes */
    if (sem_init(&shared_stuff->semlock_request, 1, 0) == -1) {
        perror("[PARENT] Failed to initialize SEM REQ");
    }
    if (sem_init(&shared_stuff->semlock_reply, 1, 0) == -1) {
        perror("[PARENT] Failed to initialize SEM REPLY");
    }
    if (sem_init(&shared_stuff->semlock_trans, 1, 1) == -1) {
        perror("[PARENT] Failed to initialize SEM TRANS");
    }

    sem_state(&shared_stuff->semlock_request, &shared_stuff->semlock_reply, &shared_stuff->semlock_trans);

    /* fork K children */
    for (int64_t child = 0; child < K; ++child) {

        /* Test process able to be created */
        if ((pids[child] = fork()) < 0) {
            perror("[PARENT] Failed to create process");
            exit(EXIT_FAILURE);
        }

        /* Call the worker */
        if (pids[child] == 0) {
            child_worker(child, filesLines, N);
            exit(0);
        }
    }

    /* Server: Service N*K Requests */
    int32_t counter = 0;
    while (counter < (N*K)) {
        //sem_state(&shared_s//tuff->semlock_request, &shared_stuff->semlock_reply, &shared_stuff->semlock_trans);

        /************************** Entry section ****************************/
        // Wait for a request
        while (sem_wait(&shared_stuff->semlock_request) == -1)
            if(errno != EINTR) {
                fprintf(stderr, "[Parent] Process failed to lock semaphore\n");
                exit(EXIT_FAILURE);
            }
        //sem_state(&shared_stuff->semlock_request, &shared_stuff->semlock_reply, &shared_stuff->semlock_trans);
        //fprintf(stderr, "[SERVER] Got a request!\n");

        /************************** Critical Section *************************/

        int64_t request = shared_stuff->request_line;
        //fprintf(stderr, "[SERVER] Got request for line %ld\n", request);

        /* Send Reply */
        char helper[100];
        getFileLine(fp, request, helper);
        //fprintf(stderr, "Which corresponds to line \"%s\"", helper);
        strncpy(shared_stuff->line, helper, 100);

        /* Notify Reply */
        sem_post(&shared_stuff->semlock_reply);

        /************************** Exit section *****************************/

        /* Reset Shared Mem State */
        //shared_stuff->request_line = -1;
        ++counter;
    }

    /* Wait */
    for (int64_t child = 0; child < N; ++child) {
        //fprintf(stderr, "Child %ld DIED\n", child+1);
        wait(0);
    }

    //fprintf(stderr, "[PARENT] SUCCESS\n");

    /* Shared memory semaphore detach */
    if (shmdt(shared_memory) == -1) {
        fprintf(stderr, "[PARENT] shmdt for SHM failed\n");
        exit(EXIT_FAILURE);
    }

    /* Play with fire */
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        fprintf(stderr, "[PARENT] shmctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }

    fclose(fp);

    return 0;
}