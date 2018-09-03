#line 123 "/home/qiang/src/splash2/splash2-master/codes/null_macros/c.m4.null.POSIX_sel4"

#line 1 "memory.C"
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#include <float.h>
#include "defs.h"
#include "memory.h"


#line 21
#include <unistd.h>
#line 21
#include <stdlib.h>
#line 21
#include <malloc.h>
#line 21
#include <sel4bench/sel4bench.h>
#line 21


g_mem *G_Memory;
local_memory Local[MAX_PROCS];

/*
 *  InitGlobalMemory ()
 *
 *  Args : none.
 *
 *  Returns : nothing.
 *
 *  Side Effects : Allocates all the global storage for G_Memory.
 *
 */
void
InitGlobalMemory ()
{
   G_Memory = (g_mem *) valloc(sizeof(g_mem));;
   G_Memory->i_array = (long *) valloc(Number_Of_Processors * sizeof(long));;
   G_Memory->d_array = (double *) valloc(Number_Of_Processors * sizeof(double));;
   if (G_Memory == NULL) {
      printf("Ran out of global memory in InitGlobalMemory\n");
      exit(-1);
   }
   G_Memory->count = 0;
   G_Memory->id = 0;
   {;};
   {;};
   {;};
   {;};
   {
#line 52
	;
#line 52
};
   ;;
   G_Memory->max_x = -MAX_REAL;
   G_Memory->min_x = MAX_REAL;
   G_Memory->max_y = -MAX_REAL;
   G_Memory->min_y = MAX_REAL;
}


