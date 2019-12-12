//
//  msgsend.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "../unpipc.c"
#include <sys/msgbuf.h>
int
main(int argc, char **argv)
{
    int        mqid;
    size_t    len;
    long    type;
    struct mymsg    *ptr;

    if (argc != 4)
        err_quit("usage: msgsnd <pathname> <#bytes> <type>");
    len = atoi(argv[2]);
    type = atoi(argv[3]);

    mqid = msgget(ftok(argv[1], 0), MSG_W);

    ptr = calloc(sizeof(long) + len, sizeof(char));
    ptr->mtype = type;

    msgsnd(mqid, ptr, len, 0);

    exit(0);
}
