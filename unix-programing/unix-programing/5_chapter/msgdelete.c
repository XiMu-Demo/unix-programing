//
//  msgdelete.c
//  unix-programing
//
//  Created by yaya on 2019/12/11.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "../unpipc.c"

int
main(int argc, char **argv)
{
    int        mqid;

    if (argc != 2)
        err_quit("usage: msgrmid <pathname>");

    mqid = msgget(ftok(argv[1], 0), 0);
    msgctl(mqid, IPC_RMID, NULL);

    exit(0);
}
