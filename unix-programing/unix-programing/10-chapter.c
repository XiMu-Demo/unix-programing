//
//  10-chapter.c
//  unix-programing
//
//  Created by yaya on 2019/12/16.
//  Copyright © 2019 西木柚子. All rights reserved.
//


//POSIX  信号量


#include "unpipc.c"

#define    MAXNTHREADS    100
#define    NBUFF     10
#define    sem_MUTEX    "mutex"         /* these are args to px_ipc_name() */
#define    sem_NEMPTY    "nempty"
#define    sem_NSTORED    "nstored"

struct {    /* data shared by producers and consumers */
  int    buff[NBUFF];
  int    nput;            /* item number: 0, 1, 2, ... */
  int    nputval;        /* value to store in buff[] */
  int    nget;            /* item number: 0, 1, 2, ... */
  int    ngetval;        /* value fetched from buff[] */
  sem_t    *mutex, *nempty, *nstored;        /* semaphores, not pointers */
}shared;

int        nitems, nproducers, nconsumers;        /* read-only */


//MARK:- 单个生产者,单个消费者
/* include prodcons */
void *
produce_1(void *arg)
{
    int        i;

    for (i = 0; i < nitems; i++) {
        sem_wait(shared.nempty);    /* wait for at least 1 empty slot */
        sem_wait(shared.mutex);
        shared.buff[i % NBUFF] = i;    /* store i into circular buffer */
        printf("    produce：buff[%d] = %d\n", i, shared.buff[i % NBUFF]);
        sem_post(shared.mutex);
        sem_post(shared.nstored);    /* 1 more stored item */
    }
    return(NULL);
}

void *
consume_1(void *arg)
{
    int        i;

    for (i = 0; i < nitems; i++) {
        sem_wait(shared.nstored);        /* wait for at least 1 stored item */
        sem_wait(shared.mutex);
        printf("consume: buff[%d] = %d\n", i, shared.buff[i % NBUFF]);
        if (shared.buff[i % NBUFF] != i)
            printf("buff[%d] = %d\n", i, shared.buff[i % NBUFF]);
        sem_post(shared.mutex);
        sem_post(shared.nempty);        /* 1 more empty slot */
    }
    return(NULL);
}

void
test_single_producer_comsumr(int argc, char **argv)
{
    pthread_t    tid_produce, tid_consume;

    if (argc != 2)
        err_quit("usage: prodcons1 <#items>");
    nitems = atoi(argv[1]);

        /* 4create three semaphores */
    shared.mutex = sem_open(Px_ipc_name(sem_MUTEX), O_CREAT | O_EXCL,
                            FILE_MODE, 1);
    shared.nempty = sem_open(Px_ipc_name(sem_NEMPTY), O_CREAT | O_EXCL,
                             FILE_MODE, NBUFF);
    shared.nstored = sem_open(Px_ipc_name(sem_NSTORED), O_CREAT | O_EXCL,
                              FILE_MODE, 0);
    
    //MACOS 已经废弃了sem_init和sem_destroy，而且这两个函数在macos上面运行有问题
//    sem_init(shared.mutex, 0, 1);
//    sem_init(shared.nempty, 0, NBUFF);
//    sem_init(shared.nstored, 0, 0);

    Set_concurrency(2);
    pthread_create(&tid_produce, NULL, produce_1, NULL);
    pthread_create(&tid_consume, NULL, consume_1, NULL);

    pthread_join(tid_produce, NULL);
    pthread_join(tid_consume, NULL);

    sem_unlink(Px_ipc_name(sem_MUTEX));
    sem_unlink(Px_ipc_name(sem_NEMPTY));
    sem_unlink(Px_ipc_name(sem_NSTORED));
    
//    sem_destroy(shared.mutex);
//    sem_destroy(shared.nempty);
//    sem_destroy(shared.nstored);
    
}



//MARK:- 多个producer，单个consumer

void *
produce_2(void *arg)
{
    for ( ; ; ) {
        sem_wait(shared.nempty);    /* wait for at least 1 empty slot */
        sem_wait(shared.mutex);

        if (shared.nput >= nitems) {
            sem_post(shared.nempty);
            sem_post(shared.mutex);
            return(NULL);            /* all done */
        }

        shared.buff[shared.nput % NBUFF] = shared.nputval;
        shared.nput++;
        shared.nputval++;

        sem_post(shared.mutex);
        sem_post(shared.nstored);    /* 1 more stored item */
        *((int *) arg) += 1;
    }
}


void *
consume_2(void *arg)
{
    int        i;

    for (i = 0; i < nitems; i++) {
        sem_wait(shared.nstored);        /* wait for at least 1 stored item */
        sem_wait(shared.mutex);

        if (shared.buff[i % NBUFF] != i)
            printf("error: buff[%d] = %d\n", i, shared.buff[i % NBUFF]);

        sem_post(shared.mutex);
        sem_post(shared.nempty);        /* 1 more empty slot */
    }
    return(NULL);
}

void
test_multi_producer_single_consumer(int argc, char **argv)
{
    int        i, count[MAXNTHREADS];
    pthread_t    tid_produce[MAXNTHREADS], tid_consume;

    if (argc != 3)
        err_quit("usage: prodcons3 <#items> <#producers>");
    nitems = atoi(argv[1]);
    nproducers = min(atoi(argv[2]), MAXNTHREADS);
    
    shared.mutex = sem_open(Px_ipc_name(sem_MUTEX), O_CREAT | O_EXCL,
                            FILE_MODE, 1);
    shared.nempty = sem_open(Px_ipc_name(sem_NEMPTY), O_CREAT | O_EXCL,
                             FILE_MODE, NBUFF);
    shared.nstored = sem_open(Px_ipc_name(sem_NSTORED), O_CREAT | O_EXCL,
                              FILE_MODE, 0);
    

        /* 4create all producers and one consumer */
    Set_concurrency(nproducers + 1);
    for (i = 0; i < nproducers; i++) {
        count[i] = 0;
        pthread_create(&tid_produce[i], NULL, produce_2, &count[i]);
    }
    pthread_create(&tid_consume, NULL, consume_2, NULL);

        /* 4wait for all producers and the consumer */
    for (i = 0; i < nproducers; i++) {
        pthread_join(tid_produce[i], NULL);
        printf("count[%d] = %d\n", i, count[i]);
    }
    pthread_join(tid_consume, NULL);

    sem_unlink(Px_ipc_name(sem_MUTEX));
    sem_unlink(Px_ipc_name(sem_NEMPTY));
    sem_unlink(Px_ipc_name(sem_NSTORED));
    exit(0);
}



//MARK:- 多个producer，多个consumer

void *
produce_3(void *arg)
{
    for ( ; ; ) {
        sem_wait(shared.nempty);    /* wait for at least 1 empty slot */
        sem_wait(shared.mutex);

        if (shared.nput >= nitems) {
            sem_post(shared.nstored);    /* let consumers terminate */
            sem_post(shared.nempty);
            sem_post(shared.mutex);
            return(NULL);            /* all done */
        }

        shared.buff[shared.nput % NBUFF] = shared.nputval;
        shared.nput++;
        shared.nputval++;

        sem_post(shared.mutex);
        sem_post(shared.nstored);    /* 1 more stored item */
        *((int *) arg) += 1;
    }
}
/* end produce */

/* include consume */
void *
consume_3(void *arg)
{
    int        i;

    for ( ; ; ) {
        sem_wait(shared.nstored);    /* wait for at least 1 stored item */
        sem_wait(shared.mutex);

        if (shared.nget >= nitems) {
            sem_post(shared.nstored);
            sem_post(shared.mutex);
            return(NULL);            /* all done */
        }

        i = shared.nget % NBUFF;
        if (shared.buff[i] != shared.ngetval)
            printf("error: buff[%d] = %d\n", i, shared.buff[i]);
        shared.nget++;
        shared.ngetval++;

        sem_post(shared.mutex);
        sem_post(shared.nempty);    /* 1 more empty slot */
        *((int *) arg) += 1;
    }
}


void
test_multi_producer_multi_consumer(int argc, char **argv)
{
    int        i, prodcount[MAXNTHREADS], conscount[MAXNTHREADS];
    pthread_t    tid_produce[MAXNTHREADS], tid_consume[MAXNTHREADS];

    if (argc != 4)
        err_quit("usage: prodcons4 <#items> <#producers> <#consumers>");
    nitems = atoi(argv[1]);
    nproducers = min(atoi(argv[2]), MAXNTHREADS);
    nconsumers = min(atoi(argv[3]), MAXNTHREADS);

        /* 4initialize three semaphores */
    shared.mutex = sem_open(Px_ipc_name(sem_MUTEX), O_CREAT | O_EXCL,
                               FILE_MODE, 1);
    shared.nempty = sem_open(Px_ipc_name(sem_NEMPTY), O_CREAT | O_EXCL,
                                FILE_MODE, NBUFF);
    shared.nstored = sem_open(Px_ipc_name(sem_NSTORED), O_CREAT | O_EXCL,
                                 FILE_MODE, 0);

        /* 4create all producers and all consumers */
    Set_concurrency(nproducers + nconsumers);
    for (i = 0; i < nproducers; i++) {
        prodcount[i] = 0;
        pthread_create(&tid_produce[i], NULL, produce_3, &prodcount[i]);
    }
    for (i = 0; i < nconsumers; i++) {
        conscount[i] = 0;
        pthread_create(&tid_consume[i], NULL, consume_3, &conscount[i]);
    }

        /* 4wait for all producers and all consumers */
    printf("\n******************** producer ***********************\n\n");

    for (i = 0; i < nproducers; i++) {
        pthread_join(tid_produce[i], NULL);
        printf("producer count[%d] = %d\n", i, prodcount[i]);
    }
    
    printf("\n********************* consumer **********************\n\n");
    for (i = 0; i < nconsumers; i++) {
        pthread_join(tid_consume[i], NULL);
        printf("consumer count[%d] = %d\n", i, conscount[i]);
    }

    sem_unlink(Px_ipc_name(sem_MUTEX));
    sem_unlink(Px_ipc_name(sem_NEMPTY));
    sem_unlink(Px_ipc_name(sem_NSTORED));
    printf("\n******************** end ***********************\n\n");

    exit(0);
}

//MARK:- 多个缓冲区
struct {    /* data shared by producer and consumer */
  struct {
    char    data[BUFFSIZE];            /* a buffer */
    ssize_t    n;                        /* count of #bytes in the buffer */
  } buff[NBUFF];                    /* NBUFF of these buffers/counts */
  sem_t    *mutex, *nempty, *nstored;        /* semaphores, not pointers */
} shared1;

int        fd;                            /* input file to copy to stdout */


/* end main */

/* include prodcons */
void *
produce_4(void *arg)
{
    int        i;

    for (i = 0; ; ) {
        sem_wait(shared1.nempty);    /* wait for at least 1 empty slot */

        sem_wait(shared1.mutex);
            /* 4critical region */
        sem_post(shared1.mutex);

        shared1.buff[i].n = read(fd, shared1.buff[i].data, 10);

        if (shared1.buff[i].n == 0) {
            sem_post(shared1.nstored);    /* 1 more stored item */
            return(NULL);
        }
        if (++i >= NBUFF)
            i = 0;                    /* circular buffer */

        sem_post(shared1.nstored);    /* 1 more stored item */
    }
    
}

void *
consume_4(void *arg)
{
    int        i;
    for (i = 0; ; ) {
        sem_wait(shared1.nstored);        /* wait for at least 1 stored item */

        sem_wait(shared1.mutex);
            /* 4critical region */
        sem_post(shared1.mutex);

        if (shared1.buff[i].n == 0){
            return(NULL);
        }
        write(STDOUT_FILENO, shared1.buff[i].data, shared1.buff[i].n);
        if (++i >= NBUFF)
            i = 0;                    /* circular buffer */
        
        sem_post(shared1.nempty);        /* 1 more empty slot */
    }
}


void
test_multi_buffer(int argc, char **argv)
{
    pthread_t    tid_produce, tid_consume;

    if (argc != 2)
        err_quit("usage: mycat2 <filepath>");

    if((fd = open(argv[1], O_RDONLY)) < 0){
        err_msg("open file error");
    }
        /* 4initialize three semaphores */
    shared1.mutex = sem_open(Px_ipc_name(sem_MUTEX), O_CREAT | O_EXCL,
                               FILE_MODE, 1);
    shared1.nempty = sem_open(Px_ipc_name(sem_NEMPTY), O_CREAT | O_EXCL,
                                FILE_MODE, NBUFF);
    shared1.nstored = sem_open(Px_ipc_name(sem_NSTORED), O_CREAT | O_EXCL,
                                 FILE_MODE, 0);
    
        /* 4one producer thread, one consumer thread */
    Set_concurrency(2);
    pthread_create(&tid_produce, NULL, produce_4, NULL);    /* reader thread */
    pthread_create(&tid_consume, NULL, consume_4, NULL);    /* writer thread */

    pthread_join(tid_produce, NULL);
    pthread_join(tid_consume, NULL);

    sem_unlink(Px_ipc_name(sem_MUTEX));
    sem_unlink(Px_ipc_name(sem_NEMPTY));
    sem_unlink(Px_ipc_name(sem_NSTORED));
    
    exit(0);
}
//MARK:- main
int
main(int argc, char **argv)
{
    test_single_producer_comsumr(argc, argv);
    return 0;
}
