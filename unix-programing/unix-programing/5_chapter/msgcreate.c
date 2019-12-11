//
//  msgcreate.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "../unpipc.c"

int
main(int argc, char **argv)
{
    int        c, oflag, mqid;

    oflag = SVMSG_MODE | IPC_CREAT;
    while ( (c = getopt(argc, argv, "e")) != -1) {
        switch (c) {
        case 'e':
            oflag |= IPC_EXCL;
            break;
        }
    }
    if (optind != argc - 1)
        err_quit("usage: msgcreate [ -e ] <pathname>");

    mqid = msgget(ftok(argv[optind], 0), oflag);
    exit(0);
}
