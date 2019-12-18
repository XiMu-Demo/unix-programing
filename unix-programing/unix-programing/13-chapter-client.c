//
//  13-chapter-client.c
//  unix-programing
//
//  Created by yaya on 2019/12/18.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "unpipc.c"


//MARK:- 给一个共享计数器持续加一

#define    MESGSIZE    256        /* max #bytes per message, incl. null at end */
#define    NMESG         16        /* max #messages */

struct shmstruct {        /* struct stored in shared memory */
  sem_t    mutex;            /* three Posix memory-based semaphores */
  sem_t    nempty;
  sem_t    nstored;
  int    nput;            /* index into msgoff[] for next put */
  long    noverflow;        /* #overflows by senders */
  sem_t    noverflowmutex;    /* mutex for noverflow counter */
  long    msgoff[NMESG];    /* offset in shared memory of each message */
  char    msgdata[NMESG * MESGSIZE];    /* the actual messages */
    int   count;
};
sem_t    *mutex;        /* pointer to named semaphore */


int
client_1(int argc, char **argv)
{
    int        fd, i, nloop;
    pid_t    pid;
    struct shmstruct    *ptr;

    if (argc != 4)
        err_quit("usage: client1 <shmname> <semname> <#loops>");
    nloop = atoi(argv[3]);

    fd = shm_open(Px_ipc_name(argv[1]), O_RDWR, FILE_MODE);
    ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE,
               MAP_SHARED, fd, 0);
    close(fd);

    mutex = sem_open(Px_ipc_name(argv[2]), 0);

    pid = getpid();
    for (i = 0; i < nloop; i++) {
        sem_wait(mutex);
        printf("pid %ld: %d\n", (long) pid, ptr->count++);
        sem_post(mutex);
    }
    exit(0);
}


//MARK:- 多客户端向同一个服务器发送消息

void
client_2(int argc, char **argv)
{
    int        fd, i, nloop, nusec;
    pid_t    pid;
    char    mesg[MESGSIZE];
    long    offset;
    struct shmstruct    *ptr;

    if (argc != 4)
        err_quit("usage: client2 <name> <#loops> <#usec>");
    nloop = atoi(argv[2]);
    nusec = atoi(argv[3]);

        /* 4open and map shared memory that server must create */
    fd = shm_open(Px_ipc_name(argv[1]), O_RDWR, FILE_MODE);
    ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE,
               MAP_SHARED, fd, 0);
    close(fd);

    pid = getpid();
    for (i = 0; i < nloop; i++) {
//        Sleep_us(1);
        snprintf(mesg, MESGSIZE, "pid %ld: message %d", (long) pid, i);

        if (sem_trywait(&ptr->nempty) == -1) {
            if (errno == EAGAIN) {
                sem_wait(&ptr->noverflowmutex);
                ptr->noverflow++;
                sem_post(&ptr->noverflowmutex);
                continue;
            } else
                err_sys("sem_trywait error");
        }

        sem_wait(&ptr->mutex);
        offset = ptr->msgoff[ptr->nput];
        if (++(ptr->nput) >= NMESG)
            ptr->nput = 0;        /* circular buffer */
        sem_post(&ptr->mutex);
        
        strcpy(&ptr->msgdata[offset], mesg);
        sem_post(&ptr->nstored);
    }
    exit(0);
}


//MARK:- main
int
main(int argc, char **argv)
{
    client_2(argc , argv);
}
