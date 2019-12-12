//
//  client.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//


//简单的客服-服务器模型，类似4.2，不过这里使用两个消息队列实现


#include "../unpipc.c"

#define    MAXMESGDATA    (PIPE_BUF - 2*sizeof(long))
    /* 4length of mesg_len and mesg_type */
#define    MESGHDRSIZE    (sizeof(struct mymesg) - MAXMESGDATA)

struct mymesg {
  long    mesg_len;    /* #bytes in mesg_data, can be 0 */
  long    mesg_type;    /* message type, must be > 0 */
  char    mesg_data[MAXMESGDATA];
};

ssize_t     mesg_send(int, struct mymesg *);
void        Mesg_send(int, struct mymesg *);
ssize_t     mesg_recv(int, struct mymesg *);
ssize_t     Mesg_recv(int, struct mymesg *);

void    client(int, int);

int
main(int argc, char **argv)
{
    int        readid, writeid;

        /* 4assumes server has created the queues */
    writeid = msgget(MQ_KEY1, 0);
    readid = msgget(MQ_KEY2, 0);

    client(readid, writeid);

        /* 4now we can delete the queues */
    msgctl(readid, IPC_RMID, NULL);
    msgctl(writeid, IPC_RMID, NULL);

    exit(0);
}


void
client(int readfd, int writefd)
{
    size_t    len;
    ssize_t    n;
    struct mymesg    mesg;

        /* 4read pathname */
    fgets(mesg.mesg_data, MAXMESGDATA, stdin);
    len = strlen(mesg.mesg_data);
    if (mesg.mesg_data[len-1] == '\n')
        len--;                /* delete newline from fgets() */
    mesg.mesg_len = len;
    mesg.mesg_type = 1;

        /* 4write pathname to IPC channel */
    Mesg_send(writefd, &mesg);

        /* 4read from IPC, write to standard output */
    while ( (n = Mesg_recv(readfd, &mesg)) > 0)
        write(STDOUT_FILENO, mesg.mesg_data, n);
}


ssize_t
mesg_recv(int id, struct mymesg *mptr)
{
    ssize_t    n;

    n = msgrcv(id, &(mptr->mesg_type), MAXMESGDATA, mptr->mesg_type, 0);
    mptr->mesg_len = n;        /* return #bytes of data */

    return(n);                /* -1 on error, 0 at EOF, else >0 */
}
/* end mesg_recv */

ssize_t
Mesg_recv(int id, struct mymesg *mptr)
{
    ssize_t    n;

    if ( (n = mesg_recv(id, mptr)) == -1)
        err_sys("mesg_recv error");
    return(n);
}

ssize_t
mesg_send(int id, struct mymesg *mptr)
{
    return(msgsnd(id, &(mptr->mesg_type), mptr->mesg_len, 0));
}
/* end mesg_send */

void
Mesg_send(int id, struct mymesg *mptr)
{
    ssize_t    n;

    if ( (n = msgsnd(id, &(mptr->mesg_type), mptr->mesg_len, 0)) == -1)
        err_sys("mesg_send error");
}

