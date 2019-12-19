//
//  server.c
//  unix-programing
//
//  Created by yaya on 2019/12/19.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "../../unpipc.c"
#include    "square.h"

//MARK:- 多线程服务端
bool_t
squareproc_2_svc(square_in *inp, square_out *outp, struct svc_req *rqstp)
{
    printf("thread %ld started, arg = %ld\n",
           pr_thread_id(NULL), inp->arg1);
    sleep(5);
    outp->res1 = inp->arg1 * inp->arg1;
    printf("thread %ld done\n", pr_thread_id(NULL));
    return(TRUE);
}

int
square_prog_2_freeresult(SVCXPRT *transp, xdrproc_t xdr_result,
                         caddr_t result)
{
    xdr_free(xdr_result, result);
    return(1);
}

