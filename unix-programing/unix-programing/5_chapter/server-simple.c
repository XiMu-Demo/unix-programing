//
//  server.c
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
void     Mesg_send(int, struct mymesg *);
ssize_t     mesg_recv(int, struct mymesg *);
ssize_t     Mesg_recv(int, struct mymesg *);

void    server(int, int);

int
main(int argc, char **argv)
{
    int        readid, writeid;
    readid = msgget(MQ_KEY1, SVMSG_MODE | IPC_CREAT);
    writeid = msgget(MQ_KEY2, SVMSG_MODE | IPC_CREAT);
    server(readid, writeid);
    exit(0);
}


void
server(int readfd, int writefd)
{
    FILE    *fp;
    ssize_t    n;
    struct mymesg    mesg;

        /* 4read pathname from IPC channel */
    mesg.mesg_type = 1;
    if ( (n = mesg_recv(readfd, &mesg)) == 0)
        err_quit("pathname missing");
    mesg.mesg_data[n] = '\0';    /* null terminate pathname */
    printf("receive client message:%s \n", mesg.mesg_data);
    if ( (fp = fopen(mesg.mesg_data, "r")) == NULL) {
            /* 4error: must tell client */
        snprintf(mesg.mesg_data + n, sizeof(mesg.mesg_data) - n,
                 ": can't open, %s\n", strerror(errno));
        mesg.mesg_len = strlen(mesg.mesg_data);
        Mesg_send(writefd, &mesg);

    } else {
            /* 4fopen succeeded: copy file to IPC channel */
        while (fgets(mesg.mesg_data, MAXMESGDATA, fp) != NULL) {
            mesg.mesg_len = strlen(mesg.mesg_data);
            Mesg_send(writefd, &mesg);
        }
        fclose(fp);
    }

        /* 4send a 0-length message to signify the end */
    mesg.mesg_len = 0;
    Mesg_send(writefd, &mesg);
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
