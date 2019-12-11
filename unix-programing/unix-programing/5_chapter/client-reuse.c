//
//  client-reuse.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//

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
void     Mesg_send(int, struct mymesg *);
ssize_t     mesg_recv(int, struct mymesg *);
ssize_t     Mesg_recv(int, struct mymesg *);

void    client(int, int);

int
main(int argc, char **argv)
{
    int        readid, writeid;

        /* 4server must create its well-known queue */
    writeid = msgget(MQ_KEY1, 0);
        /* 4we create our own private queue */
    readid = msgget(IPC_PRIVATE, SVMSG_MODE | IPC_CREAT);
    client(readid, writeid);
        /* 4and delete our private queue */
    msgctl(readid, IPC_RMID, NULL);
    exit(0);
}


void
client(int readid, int writeid)
{
    size_t    len;
    ssize_t    n;
    char    *ptr;
    struct mymesg    mesg;

        /* 4start buffer with msqid and a blank */
    snprintf(mesg.mesg_data, MAXMESGDATA, "%d ", readid);
    len = strlen(mesg.mesg_data);
    ptr = mesg.mesg_data + len;

        /* 4read pathname */
    fgets(ptr, MAXMESGDATA - len, stdin);
    len = strlen(mesg.mesg_data);
    if (mesg.mesg_data[len-1] == '\n')
        len--;                /* delete newline from fgets() */
    mesg.mesg_len = len;
    mesg.mesg_type = 1;

        /* 4write msqid and pathname to server's well-known queue */
    Mesg_send(writeid, &mesg);

        /* 4read from our queue, write to standard output */
    while ( (n = Mesg_recv(readid, &mesg)) > 0)
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

/* include Mesg_recv */
ssize_t
Mesg_recv(int id, struct mymesg *mptr)
{
    ssize_t    n;

    do {
        n = mesg_recv(id, mptr);
    } while (n == -1 && errno == EINTR);

    if (n == -1)
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
