//
//  4_pipe-fifo.c
//  unix-programing
//
//  Created by yaya on 2019/12/9.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include "unpipc.c"

//MARK:- PIPE

void
client(int readfd, int writefd)
{
    size_t    len;
    ssize_t    n;
    char    buff[MAXLINE];

        /* 4read pathname */
    fgets(buff, MAXLINE, stdin);
    len = strlen(buff);        /* fgets() guarantees null byte at end */
    if (buff[len-1] == '\n')
        len--;                /* delete newline from fgets() */

        /* 4write pathname to IPC channel */
    write(writefd, buff, len);

        /* 4read from IPC, write to standard output */
    while ( (n = read(readfd, buff, MAXLINE)) > 0)
        write(STDOUT_FILENO, buff, n);
}


void
server(int readfd, int writefd)
{
    int        fd;
    ssize_t    n;
    char    buff[MAXLINE+1];

        /* 4read pathname from IPC channel */
    if ( (n = read(readfd, buff, MAXLINE)) == 0)
        err_quit("end-of-file while reading pathname");
    buff[n] = '\0';        /* null terminate pathname */

    if ( (fd = open(buff, O_RDONLY)) < 0) {
            /* 4error: must tell client */
        snprintf(buff + n, sizeof(buff) - n, ": can't open, %s\n",
                 strerror(errno));
        n = strlen(buff);
        write(writefd, buff, n);

    } else {
            /* 4open succeeded: copy file to IPC channel */
        while ( (n = read(fd, buff, MAXLINE)) > 0)
            write(writefd, buff, n);
        close(fd);
    }
}


void test_pipe()
{
    int        pipe1[2], pipe2[2];
    pid_t    childpid;

    pipe(pipe1);    /* create two pipes */
    pipe(pipe2);

    if ( (childpid = fork()) == 0) {        /* child */
       close(pipe1[1]);
       close(pipe2[0]);

       server(pipe1[0], pipe2[1]);
       exit(0);
    }
       /* 4parent */
    close(pipe1[0]);
    close(pipe2[1]);

    client(pipe2[0], pipe1[1]);

    waitpid(childpid, NULL, 0);        /* wait for child to terminate */
    exit(0);
}

//MARK:- popen,pclose
void test_popen()
{
    size_t    n;
    char    buff[MAXLINE], command[MAXLINE];
    FILE    *fp;

        /* 4read pathname */
    fgets(buff, MAXLINE, stdin);
    n = strlen(buff);        /* fgets() guarantees null byte at end */
    if (buff[n-1] == '\n')
        n--;                /* delete newline from fgets() */

    snprintf(command, sizeof(command), "cat %s", buff);
    fp = popen(command, "r");

        /* 4copy from pipe to standard output */
    while (fgets(buff, MAXLINE, fp) != NULL)
        fputs(buff, stdout);

    pclose(fp);
    exit(0);
}

//MARK:- fifo

//有亲缘关系的fifo
void test_relationship_fifo()
{
    int        readfd, writefd;
    pid_t    childpid;

        /* 4create two FIFOs; OK if they already exist */
    if ((mkfifo(FIFO1, FILE_MODE) < 0) && (errno != EEXIST))
        err_sys("can't create %s", FIFO1);
    if ((mkfifo(FIFO2, FILE_MODE) < 0) && (errno != EEXIST)) {
        unlink(FIFO1);
        err_sys("can't create %s", FIFO2);
    }

    if ( (childpid = fork()) == 0) {        /* child */
        readfd = open(FIFO1, O_RDONLY, 0);
        writefd = open(FIFO2, O_WRONLY, 0);

        server(readfd, writefd);
        exit(0);
    }
        /* 4parent */
    writefd = open(FIFO1, O_WRONLY, 0);
    readfd = open(FIFO2, O_RDONLY, 0);

    client(readfd, writefd);

    waitpid(childpid, NULL, 0);        /* wait for child to terminate */

    close(readfd);
    close(writefd);

    unlink(FIFO1);
    unlink(FIFO2);
    exit(0);
}


//无亲缘关系的fifo
void test_no_relationship_fifo()
{
    int        readfd, writefd;

        /* 4create two FIFOs; OK if they already exist */
    if ((mkfifo(FIFO1, FILE_MODE) < 0) && (errno != EEXIST))
        err_sys("can't create %s", FIFO1);
    if ((mkfifo(FIFO2, FILE_MODE) < 0) && (errno != EEXIST)) {
        unlink(FIFO1);
        err_sys("can't create %s", FIFO2);
    }

    readfd = open(FIFO1, O_RDONLY, 0);
    writefd = open(FIFO2, O_WRONLY, 0);

    server(readfd, writefd);
    exit(0);
}

//单个服务器，多个客服端fifo
void test_multi_server_fifo()
{
    int        readfifo, writefifo, dummyfd, fd;
    char    *ptr, buff[MAXLINE], fifoname[MAXLINE];
    pid_t    pid;
    ssize_t    n;
    
        /* 4create server's well-known FIFO; OK if already exists */
    if ((mkfifo(SERV_FIFO, FILE_MODE) < 0) && (errno != EEXIST))
        err_sys("can't create %s", SERV_FIFO);

        /* 4open server's well-known FIFO for reading and writing */
    readfifo = open(SERV_FIFO, O_RDONLY, 0);
    dummyfd = open(SERV_FIFO, O_WRONLY, 0);        /* never used */

    while ( (n = readline(readfifo, buff, MAXLINE)) > 0) {
        
        if (buff[n-1] == '\n')
            n--;            /* delete newline from readline() */
        buff[n] = '\0';        /* null terminate pathname */

        if ( (ptr = strchr(buff, ' ')) == NULL) {
            err_msg("bogus request: %s", buff);
            continue;
        }
        printf("buff:%s--ptr:%s \n", buff,ptr);

        *ptr++ = 0;            /* null terminate PID, ptr = pathname */
        pid = atoi(buff);
        printf("buff:%s--ptr:%s \n", buff,ptr);
        snprintf(fifoname, sizeof(fifoname), "/tmp/fifo.%ld", (long) pid);
        if ( (writefifo = open(fifoname, O_WRONLY, 0)) < 0) {
            err_msg("cannot open: %s", fifoname);
            continue;
        }

        if ( (fd = open(ptr, O_RDONLY)) < 0) {
                /* 4error: must tell client */
            snprintf(buff + n, sizeof(buff) - n, ": can't open, %s\n",
                     strerror(errno));
            n = strlen(ptr);
            write(writefifo, ptr, n);
            close(writefifo);
    
        } else {
                /* 4open succeeded: copy file to FIFO */
            while ( (n = read(fd, buff, MAXLINE)) > 0)
                write(writefifo, buff, n);
            close(fd);
            close(writefifo);
        }
    }
}

//MARK:- 字节流与消息
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

ssize_t
mesg_recv(int fd, struct mymesg *mptr)
{
    size_t    len;
    ssize_t    n;

        /* 4read message header first, to get len of data that follows */
    if ( (n = read(fd, mptr, MESGHDRSIZE)) == 0)
        return(0);        /* end of file */
    else if (n != MESGHDRSIZE)
        err_quit("message header: expected %d, got %d", MESGHDRSIZE, n);

    if ( (len = mptr->mesg_len) > 0)
        if ( (n = read(fd, mptr->mesg_data, len)) != len)
            err_quit("message data: expected %d, got %d", len, n);
    return(len);
}

ssize_t
mesg_send(int fd, struct mymesg *mptr)
{
    return(write(fd, mptr, MESGHDRSIZE + mptr->mesg_len));
}

void
server_1(int readfd, int writefd)
{
    FILE    *fp;
    ssize_t    n;
    struct mymesg    mesg;

        /* 4read pathname from IPC channel */
    mesg.mesg_type = 1;
    if ( (n = mesg_recv(readfd, &mesg)) == 0)
        err_quit("pathname missing");
    mesg.mesg_data[n] = '\0';    /* null terminate pathname */
    printf("receive message:%s--length:%ld", mesg.mesg_data,mesg.mesg_len);
    if ( (fp = fopen(mesg.mesg_data, "r")) == NULL) {
            /* 4error: must tell client */
        snprintf(mesg.mesg_data + n, sizeof(mesg.mesg_data) - n,
                 ": can't open, %s\n", strerror(errno));
        mesg.mesg_len = strlen(mesg.mesg_data);
        mesg_send(writefd, &mesg);

    } else {
            /* 4fopen succeeded: copy file to IPC channel */
        while (fgets(mesg.mesg_data, MAXMESGDATA, fp) != NULL) {
            mesg.mesg_len = strlen(mesg.mesg_data);
            mesg_send(writefd, &mesg);
        }
        fclose(fp);
    }

        /* 4send a 0-length message to signify the end */
    mesg.mesg_len = 0;
    mesg_send(writefd, &mesg);
}


void
client_1(int readfd, int writefd)
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
    mesg_send(writefd, &mesg);

        /* 4read from IPC, write to standard output */
    while ( (n = mesg_recv(readfd, &mesg)) > 0)
        write(STDOUT_FILENO, mesg.mesg_data, n);
}


void test_stream()
{
    int        pipe1[2], pipe2[2];
    pid_t    childpid;

    pipe(pipe1);    /* create two pipes */
    pipe(pipe2);

    if ( (childpid = fork()) == 0) {        /* child */
       close(pipe1[1]);
       close(pipe2[0]);
       client_1(pipe1[0], pipe2[1]);
       exit(0);
    }
       /* 4parent */
    close(pipe1[0]);
    close(pipe2[1]);
    server_1(pipe2[0], pipe1[1]);
    waitpid(childpid, NULL, 0);   /* wait for child to terminate */
    exit(0);
}


//MARK:- main

int main(int argc, char **argv)
{
    test_stream();
    return 0;
}
