//
//  9-chapter-2.c
//  unix-programing
//
//  Created by yaya on 2019/12/20.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include <stdio.h>

#include    "../unpipc.c"

#define    SEQFILE    "/Users/yaya/学习/unix-programing/unix-programing/unix-programing/file"        /* filename */


//MARK:- 测试fcntl记录上锁是劝告性质的

/*同时运行 9-chapter-1 和 9-chapter-2两个程序，最后发现最终数字不是201，这是因为1是记录上锁，2是没有上锁，这样即使1在上锁期间，
 2还是可以获取该锁进行读写操作，也就造成了问题.
 一部分输出如下：
 /unlock: pid = 55567, seq# = 109
 ./unlock: pid = 55567, seq# = 110
 ./unlock: pid = 55567, seq# = 111
 ./lock: pid = 55566, seq# = 111
 ./unlock: pid = 55567, seq# = 112
 ./lock: pid = 55566, seq# = 113
 ./unlock: pid = 55567, seq# = 113
 ./unlock: pid = 55567, seq# = 114
 ./lock: pid = 55566, seq# = 114
 ./unlock: pid = 55567, seq# = 115
 ./lock: pid = 55566, seq# = 115
 ./unlock: pid = 55567, seq# = 116
 ./lock: pid = 55566, seq# = 116
 ./lock: pid = 55566, seq# = 117
 ./lock: pid = 55566, seq# = 118
可以看到lock和unlock写入了相同的数字
 */


void
my_lock_none(int fd)
{
    return;
}

void
my_unlock_none(int fd)
{
    return;
}


void test_record_lock_advise(int argc, char **argv)
{
    int        fd;
       long    i, seqno;
       pid_t    pid;
       ssize_t    n;
       char    line[MAXLINE + 1];

       pid = getpid();
       fd = open(SEQFILE, O_RDWR, FILE_MODE);

       for (i = 0; i < 100; i++) {
           my_lock_none(fd);                /* lock the file */

           lseek(fd, 0L, SEEK_SET);    /* rewind before read */
           n = read(fd, line, MAXLINE);
           line[n] = '\0';                /* null terminate for sscanf */

           n = sscanf(line, "%ld\n", &seqno);
           printf("%s: pid = %ld, seq# = %ld\n", argv[0], (long) pid, seqno);

           seqno++;                    /* increment sequence number */

           snprintf(line, sizeof(line), "%ld\n", seqno);
           lseek(fd, 0L, SEEK_SET);    /* rewind before write */
           write(fd, line, strlen(line));

           my_unlock_none(fd);                /* unlock the file */
       }
}

int
main(int argc, char **argv)
{
    test_record_lock_advise(argc, argv);
    exit(0);
}
