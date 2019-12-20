//
//  server.c
//  unix-programing
//
//  Created by yaya on 2019/12/19.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include    "../../unpipc.c"
#include    "square.h"


//MARK:- 单线程服务端
square_out *
squareproc_1_svc(square_in *inp, struct svc_req *rqstp)
{
    static square_out    out;

    out.res1 = inp->arg1 * inp->arg1;
    return(&out);
}



