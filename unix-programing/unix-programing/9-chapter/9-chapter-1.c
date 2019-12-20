//
//  9-chapter.c
//  unix-programing
//
//  Created by yaya on 2019/12/20.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "../unpipc.c"

#define    SEQFILE    "/Users/yaya/学习/unix-programing/unix-programing/unix-programing/file"        /* filename */


//MARK:- fcntl记录上锁

void
my_lock(int fd)
{
    struct flock    lock;

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;                /* write lock entire file */

    fcntl(fd, F_SETLKW, &lock);
}

void
my_unlock(int fd)
{
    struct flock    lock;

    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;                /* unlock entire file */

    fcntl(fd, F_SETLK, &lock);
}


int
main(int argc, char **argv)
{
    int        fd;
    long    i, seqno;
    pid_t    pid;
    ssize_t    n;
    char    line[MAXLINE + 1];

    pid = getpid();
    fd = open(SEQFILE, O_RDWR, FILE_MODE);

    for (i = 0; i < 100; i++) {
        my_lock(fd);                /* lock the file */

        lseek(fd, 0L, SEEK_SET);    /* rewind before read */
        n = read(fd, line, MAXLINE);
        line[n] = '\0';                /* null terminate for sscanf */

        n = sscanf(line, "%ld\n", &seqno);
        printf("%s: pid = %ld, seq# = %ld\n", argv[0], (long) pid, seqno);

        seqno++;                    /* increment sequence number */

        snprintf(line, sizeof(line), "%ld\n", seqno);
        lseek(fd, 0L, SEEK_SET);    /* rewind before write */
        write(fd, line, strlen(line));

        my_unlock(fd);                /* unlock the file */
    }    exit(0);
}
