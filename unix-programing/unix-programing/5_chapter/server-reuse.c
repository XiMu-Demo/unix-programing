//
//  server-reuse.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//

/*
 使用单个消息队列实现单服务器多客服端模型，客户端到服务器使用同一个队列，服务器到客户端使用不同队列，使用一
 个众所周知的消息类型
(比如1)表示客户到服务端的消息，客户端使用IPC_PRIVATE创建各自的队列，然后客户端把自己的私有队列标识符传递给服务端，服务器就在接受到队列中写入自己的应答
 */

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

void    server(int, int);

int
main(int argc, char **argv)
{
    int        msqid;
    msqid = msgget(MQ_KEY1, SVMSG_MODE | IPC_CREAT);
    server(msqid, msqid);    /* same queue for both directions */
    exit(0);
}


void
server(int readid, int writeid)
{
    FILE    *fp;
    char    *ptr;
    ssize_t    n;
    struct mymesg    mesg;
    void    sig_chld(int);

    Signal(SIGCHLD, sig_chld);

    for ( ; ; ) {
            /* 4read pathname from our well-known queue */
        mesg.mesg_type = 1;
        if ( (n = Mesg_recv(readid, &mesg)) == 0) {
            err_msg("pathname missing");
            continue;
        }
        mesg.mesg_data[n] = '\0';    /* null terminate pathname */

        if ( (ptr = strchr(mesg.mesg_data, ' ')) == NULL) {
            err_msg("bogus request: %s", mesg.mesg_data);
            continue;
        }
        *ptr++ = 0;            /* null terminate msgid, ptr = pathname */
        writeid = atoi(mesg.mesg_data);

        if (fork() == 0) {        /* child */
            if ( (fp = fopen(ptr, "r")) == NULL) {
                    /* 4error: must tell client */
                snprintf(mesg.mesg_data + n, sizeof(mesg.mesg_data) - n,
                         ": can't open, %s\n", strerror(errno));
                mesg.mesg_len = strlen(ptr);
                memmove(mesg.mesg_data, ptr, mesg.mesg_len);
                Mesg_send(writeid, &mesg);
        
            } else {
                    /* 4fopen succeeded: copy file to client's queue */
                while (fgets(mesg.mesg_data, MAXMESGDATA, fp) != NULL) {
                    mesg.mesg_len = strlen(mesg.mesg_data);
                    Mesg_send(writeid, &mesg);
                }
                fclose(fp);
            }
        
                /* 4send a 0-length message to signify the end */
            mesg.mesg_len = 0;
            Mesg_send(writeid, &mesg);
            exit(0);        /* child terminates */
        }
        /* parent just loops around */
    }
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
