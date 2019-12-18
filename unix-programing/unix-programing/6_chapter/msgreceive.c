//
//  msgreceive.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "../unpipc.c"

#define    MAXMSG    (8192 + sizeof(long))

int
main(int argc, char **argv)
{
    int        c, flag, mqid;
    long    type;
    ssize_t    n;
    struct mymsg    *buff;

    type = flag = 0;
    while ( (c = getopt(argc, argv, "nt:")) != -1) {
        switch (c) {
        case 'n':
            flag |= IPC_NOWAIT;
            break;

        case 't':
            type = atol(optarg);
            break;
        }
    }
    printf("%d--%d",optind, argc);

    if (optind != argc - 1)
        err_quit("usage: msgrcv [ -n ] [ -t type ] <pathname>");
    mqid = msgget(ftok(argv[optind], 0), MSG_R);

    buff = malloc(MAXMSG);

    n = msgrcv(mqid, buff, MAXMSG, type, flag);
    printf("read %zd bytes, type = %ld\n", n, buff->mtype);

    exit(0);
}
