//
//  12-chapter.c
//  unix-programing
//
//  Created by yaya on 2019/12/17.
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
server_1(int argc, char **argv)
{
    int        fd;
    struct shmstruct    *ptr;

    if (argc != 3)
        err_quit("usage: server1 <shmname> <semname>");

    shm_unlink(Px_ipc_name(argv[1]));        /* OK if this fails */
        /* 4create shm, set its size, map it, close descriptor */
    fd = shm_open(Px_ipc_name(argv[1]), O_RDWR | O_CREAT | O_EXCL, FILE_MODE);
    ftruncate(fd, sizeof(struct shmstruct));
    ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE,
               MAP_SHARED, fd, 0);
    close(fd);

    sem_unlink(Px_ipc_name(argv[2]));        /* OK if this fails */
    mutex = sem_open(Px_ipc_name(argv[2]), O_CREAT | O_EXCL, FILE_MODE, 1);
    sem_close(mutex);

    exit(0);
}

//MARK:- 多客户端向同一个服务器发送消息
void
server_2(int argc, char **argv)
{
    int         fd, index;
    long        lastnoverflow,temp;
    long        offset;
    struct shmstruct    *ptr;

    if (argc != 2)
        err_quit("usage: server2 <name>");

        /* 4create shm, set its size, map it, close descriptor */
    shm_unlink(Px_ipc_name(argv[1]));        /* OK if this fails */
    fd = shm_open(Px_ipc_name(argv[1]), O_RDWR | O_CREAT | O_EXCL, FILE_MODE);
    ftruncate(fd, sizeof(struct shmstruct));
    ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE,
               MAP_SHARED, fd, 0);
    close(fd);

        /* 4initialize the array of offsets */
    for (index = 0; index < NMESG; index++)
        ptr->msgoff[index] = index * MESGSIZE;

        /* 4initialize the semaphores in shared memory */
    sem_init(&ptr->mutex, 1, 1);
    sem_init(&ptr->nempty, 1, NMESG);
    sem_init(&ptr->nstored, 1, 0);
    sem_init(&ptr->noverflowmutex, 1, 1);

        /* 4this program is the consumer */
    index = 0;
    lastnoverflow = 0;
    for ( ; ; ) {
        sleep(1);
//        Sleep_us(0.1);
        sem_wait(&ptr->nstored);
        sem_wait(&ptr->mutex);
        offset = ptr->msgoff[index];
        printf("index = %d: %s\n", index, &ptr->msgdata[offset]);
        if (++index >= NMESG)
            index = 0;                /* circular buffer */
        sem_post(&ptr->mutex);
        sem_post(&ptr->nempty);

        sem_wait(&ptr->noverflowmutex);
        temp = ptr->noverflow;        /* don't printf while mutex held */
        sem_post(&ptr->noverflowmutex);
        if (temp != lastnoverflow) {
            printf("noverflow = %ld\n", temp);
            lastnoverflow = temp;
        }
    }

}




//MARK:- main

int
main(int argc, char **argv)
{
    server_2(argc , argv);
}


