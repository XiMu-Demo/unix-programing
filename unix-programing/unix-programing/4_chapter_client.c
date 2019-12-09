//
//  4_chapter_client.c
//  unix-programing
//
//  Created by yaya on 2019/12/9.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include "unpipc.c"


//MARK:- 无亲缘关系的fifo
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


void test_no_relationship_fifo()
{
    int        readfd, writefd;

    writefd = open(FIFO1, O_WRONLY, 0);
    readfd = open(FIFO2, O_RDONLY, 0);

    client(readfd, writefd);

    close(readfd);
    close(writefd);

    unlink(FIFO1);
    unlink(FIFO2);
    exit(0);
}

//MARK:- 单个服务器，多个客服端fifo
void test_multi_server_fifo()
{
    int        readfifo, writefifo;
    size_t    len;
    ssize_t    n;
    char    *ptr, fifoname[MAXLINE], buff[MAXLINE];
    pid_t    pid;

        /* 4create FIFO with our PID as part of name */
    pid = getpid();
    snprintf(fifoname, sizeof(fifoname), "/tmp/fifo.%ld", (long) pid);
    if ((mkfifo(fifoname, FILE_MODE) < 0) && (errno != EEXIST))
        err_sys("can't create %s", fifoname);

        /* 4start buffer with pid and a blank */
    snprintf(buff, sizeof(buff), "%ld ", (long) pid);
    len = strlen(buff);
    ptr = buff + len;

        /* 4read pathname */
    fgets(ptr, MAXLINE - len, stdin);
    len = strlen(buff);        /* fgets() guarantees null byte at end */

        /* 4open FIFO to server and write PID and pathname to FIFO */
    writefifo = open(SERV_FIFO, O_WRONLY, 0);
    printf("client send message:%s", buff);
    write(writefifo, buff, len);

        /* 4now open our FIFO; blocks until server opens for writing */
    readfifo = open(fifoname, O_RDONLY, 0);

        /* 4read from IPC, write to standard output */
    while ( (n = read(readfifo, buff, MAXLINE)) > 0)
        write(STDOUT_FILENO, buff, n);

    close(readfifo);
    unlink(fifoname);
    exit(0);
}



int main(int argc, char **argv)
{
    test_multi_server_fifo();
    return 0;
}
