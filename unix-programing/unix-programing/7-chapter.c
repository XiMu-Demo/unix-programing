//
//  6-chapter.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "unpipc.c"

#define    MAXNITEMS         1000000
#define    MAXNTHREADS            100

int        nitems;            /* read-only by producer and consumer */
struct {
  pthread_mutex_t    mutex;
  int    buff[MAXNITEMS];
  int    nput;
  int    nval;
} shared = { PTHREAD_MUTEX_INITIALIZER };


//MARK:- 生产-消费者问题1：先等待全部启动完生产者，在启动消费者
void *
produce_1(void *arg)
{
    printf("arg:%d \n",  *((int *) arg));
    for ( ; ; ) {
        pthread_mutex_lock(&shared.mutex);
        if (shared.nput >= nitems) {
            pthread_mutex_unlock(&shared.mutex);
            return(NULL);        /* array is full, we're done */
        }
        shared.buff[shared.nput] = shared.nval;
        shared.nput++;
        shared.nval++;
        pthread_mutex_unlock(&shared.mutex);
        *((int *) arg) += 1;
    }
}

void *
consume_1(void *arg)
{
    int        i;

    for (i = 0; i < nitems; i++) {
        if (shared.buff[i] != i)
            printf("buff[%d] = %d\n", i, shared.buff[i]);
    }
    return(NULL);
}


void test_producer_consumer_1(int argc, char **argv)
{
    int            i, nthreads, count[MAXNTHREADS];
    pthread_t    tid_produce[MAXNTHREADS], tid_consume;

    if (argc != 3)
        err_quit("usage: prodcons2 <#items> <#threads>");
    nitems = min(atoi(argv[1]), MAXNITEMS);
    nthreads = min(atoi(argv[2]), MAXNTHREADS);

    //告诉线程系统希望并发多少个线程，当希望多个线程中每条线程都有机会执行，该方法必须，macos不影响
    Set_concurrency(nthreads);
        /* 4start all the producer threads */
    for (i = 0; i < nthreads; i++) {
        count[i] = 0;
        pthread_create(&tid_produce[i], NULL, produce_1, &count[i]);
    }

        /* 4wait for all the producer threads */
    for (i = 0; i < nthreads; i++) {
        pthread_join(tid_produce[i], NULL);
        printf("count[%d] = %d\n", i, count[i]);
    }

        /* 4start, then wait for the consumer thread */
    pthread_create(&tid_consume, NULL, consume_1, NULL);
    pthread_join(tid_consume, NULL);
}


//MARK:- 生产-消费者问题2：启动完生产者立即启动消费者，不等待生产者全部启动完成，生产者一边生产数据，消费者一边处理数据


void *
produce_2(void *arg)
{
    for ( ; ; ) {
        pthread_mutex_lock(&shared.mutex);
        if (shared.nput >= nitems) {
            pthread_mutex_unlock(&shared.mutex);
            return(NULL);        /* array is full, we're done */
        }
        shared.buff[shared.nput] = shared.nval;
        shared.nput++;
        shared.nval++;
        pthread_mutex_unlock(&shared.mutex);
        *((int *) arg) += 1;
    }
}

/* include consume */
void
consume_wait_2(int i)
{
    for ( ; ; ) {
        pthread_mutex_lock(&shared.mutex);
        //等待生产者准备好数据，不然就一直轮询-等待，这种方法比较耗费cpu，好的方法是生产者先睡眠
        //等到生产者准备好了数据，直接通知给消费者，消费者醒来在处理，这样就不用轮询了
        if (i < shared.nput) {
            pthread_mutex_unlock(&shared.mutex);
            return;            /* an item is ready */
        }
        pthread_mutex_unlock(&shared.mutex);
    }
}

void *
consume_2(void *arg)
{
    int        i;

    for (i = 0; i < nitems; i++) {
        consume_wait_2(i);
        if (shared.buff[i] != i)
            printf("buff[%d] = %d\n", i, shared.buff[i]);
    }
    return(NULL);
}



void
test_producer_consumer_2(int argc, char **argv)
{
    int            i, nthreads, count[MAXNTHREADS];
    pthread_t    tid_produce[MAXNTHREADS], tid_consume;

    if (argc != 3)
        err_quit("usage: prodcons3 <#items> <#threads>");
    nitems = min(atoi(argv[1]), MAXNITEMS);
    nthreads = min(atoi(argv[2]), MAXNTHREADS);

        /* 4create all producers and one consumer */
    Set_concurrency(nthreads + 1);
    for (i = 0; i < nthreads; i++) {
        count[i] = 0;
        pthread_create(&tid_produce[i], NULL, produce_2, &count[i]);
    }
    pthread_create(&tid_consume, NULL, consume_2, NULL);

        /* 4wait for all producers and the consumer */
    for (i = 0; i < nthreads; i++) {
        pthread_join(tid_produce[i], NULL);
        printf("count[%d] = %d\n", i, count[i]);
    }
    pthread_join(tid_consume, NULL);

    exit(0);
}


//MARK:- 生产-消费者问题3：使用条件变量来实现等待-通知
//当生产者准备好了数据，就先唤醒消费者，然后设置某条件为真，消费者醒来验证该条件为真，就开始处理数据
        /* globals shared by threads */
int        nitems;                /* read-only by producer and consumer */
int        buff[MAXNITEMS];
struct {
  pthread_mutex_t    mutex;
  int                nput;    /* next index to store */
  int                nval;    /* next value to store */
} put = { PTHREAD_MUTEX_INITIALIZER };

struct {
  pthread_mutex_t    mutex;
  pthread_cond_t    cond;
  int                nready;    /* number ready for consumer */
} nready = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER };


void *
produce_3(void *arg)
{
    for ( ; ; ) {
        pthread_mutex_lock(&put.mutex);
        if (put.nput >= nitems) {
            pthread_mutex_unlock(&put.mutex);
            return(NULL);        /* array is full, we're done */
        }
        buff[put.nput] = put.nval;
        put.nput++;
        put.nval++;
        pthread_mutex_unlock(&put.mutex);

        pthread_mutex_lock(&nready.mutex);
        if (nready.nready == 0)
            pthread_cond_signal(&nready.cond);
        nready.nready++;
        pthread_mutex_unlock(&nready.mutex);

        *((int *) arg) += 1;
    }
}

void *
consume_3(void *arg)
{
    int        i;
    for (i = 0; i < nitems; i++) {
        pthread_mutex_lock(&nready.mutex);
        while (nready.nready == 0)
            pthread_cond_wait(&nready.cond, &nready.mutex);
        nready.nready--;
        pthread_mutex_unlock(&nready.mutex);

        if (buff[i] != i)
            printf("buff[%d] = %d\n", i, buff[i]);
    }
    return(NULL);
}


int
test_producer_consumer_3(int argc, char **argv)
{
    int            i, nthreads, count[MAXNTHREADS];
    pthread_t    tid_produce[MAXNTHREADS], tid_consume;

    if (argc != 3)
        err_quit("usage: prodcons6 <#items> <#threads>");
    nitems = min(atoi(argv[1]), MAXNITEMS);
    nthreads = min(atoi(argv[2]), MAXNTHREADS);

    Set_concurrency(nthreads + 1);
        /* 4create all producers and one consumer */
    for (i = 0; i < nthreads; i++) {
        count[i] = 0;
        pthread_create(&tid_produce[i], NULL, produce_3, &count[i]);
    }
    pthread_create(&tid_consume, NULL, consume_3, NULL);

        /* wait for all producers and the consumer */
    for (i = 0; i < nthreads; i++) {
        pthread_join(tid_produce[i], NULL);
        printf("count[%d] = %d\n", i, count[i]);
    }
    pthread_join(tid_consume, NULL);
    exit(0);
}




//MARK:- main
int
main(int argc, char **argv)
{
    test_producer_consumer_3(argc,argv);
    exit(0);
}
