//
//  unpipc.c
//  unix-programing
//
//  Created by yaya on 2019/12/9.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include "unpipc.h"
#include    <stdarg.h>
#include    <errno.h>        /* for definition of errno */
#include    <stdarg.h>        /* ANSI C header file */
#include    <sys/syslog.h>


//MARK:- read
ssize_t readline(int fd, void *vptr, size_t maxlen)
{
    int        n;
    ssize_t rc;
    char    c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
//        if ( (rc = _my_read(fd, &c)) == 1) {
        if ( (rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;//每次用_my_read函数读取一个char c，然后存入ptr，也就是vptr里面，然后指针右移，以便存入下一个char c
            if (c == '\n')
                break;
        } else if (rc == 0) {
            *ptr = 0;
            printf("读取到了文件末尾EOF \n");
            return(n - 1);    /* EOF, n - 1 bytes were read */
        } else{
            printf("readline error \n");
            return(-1);    /* error */
        }
    }

    *ptr = 0;
    return(n);
}

//MARK:- ERROR
int        daemon_proc;        /* set nonzero by daemon_init() */

static void    err_doit(int, int, const char *, va_list);

/* Nonfatal error related to system call
 * Print message and return */

void
err_ret(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error related to system call
 * Print message and terminate */

void
err_sys(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

void
err_dump(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    abort();        /* dump core and terminate */
}

/* Nonfatal error unrelated to system call
 * Print message and return */

void
err_msg(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error unrelated to system call
 * Print message and terminate */

void
err_quit(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
    int        errno_save;
    long        n;
    char    buf[MAXLINE + 1];

    errno_save = errno;        /* value caller might want printed */
#ifdef    HAVE_VSNPRINTF
    vsnprintf(buf, MAXLINE, fmt, ap);    /* safe */
#else
    vsprintf(buf, fmt, ap);                    /* not safe */
#endif
    n = strlen(buf);
    if (errnoflag)
        snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
    strcat(buf, "\n");

    if (daemon_proc) {
        syslog(level, "%s", buf);
    } else {
        fflush(stdout);        /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}

//MARK: - 信号处理

Sigfunc* Signal(int signo, Sigfunc* func)
{
    struct sigaction act, oldact;
    act.sa_handler = func;
    //sa_mask置空，意味着除了被捕获的信号之外的信号都不会被阻断
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    
    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else{
        /*
         一些IO系统调用执行时, 如 read 等待输入期间, 如果收到一个信号,系统将中断read, 转而执行信号处理函数. 当信号处理返回后, 系统遇到了一个问题:
         是重新开始这个系统调用, 还是让系统调用失败?早期UNIX系统的做法是, 中断系统调用, 并让系统调用失败, 比如read返回 -1, 同时设置 errno 为
         EINTR, 但是中断了的系统调用是没有完成的调用, 它的失败是临时性的, 如果再次调用则可能成功, 这并不是真正的失败, 所以要对这种情况进行处理:
         我们可以从信号的角度来解决这个问题,  安装信号的时候, 设置 SA_RESTART属性, 那么当信号处理函数返回后, 被该信号中断的系统调用将自动恢复.
         */
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    //oldact保存信号中断之前的设置的信号处理函数的指针，也就是act的信息
    if (sigaction(signo, &act, &oldact) < 0) {
        return SIG_ERR;
    }
    return oldact.sa_handler;
    
}


void sig_chld(int signo)
{
    pid_t    pid;
    int        stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated\n", pid);
    }
    return;
}

//MARK:- 多线程
int
set_concurrency(int level)
{
    // vonzhou
    // on Linux we have pthread_setconcurrency
    return(pthread_setconcurrency(level));
}

void
Set_concurrency(int level)
{
    if (set_concurrency(level) != 0)
        err_sys("set_concurrency error");
}
