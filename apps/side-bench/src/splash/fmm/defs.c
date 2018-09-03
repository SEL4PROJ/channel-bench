#line 123 "/home/qiang/src/splash2/splash2-master/codes/null_macros/c.m4.null.POSIX_sel4"

#line 1 "defs.C"
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

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "defs.h"
#include "memory.h"

long Number_Of_Processors;
double Timestep_Dur;
real Softening_Param;
long Expansion_Terms;


real
RoundReal (real val)
{
   double shifter;
   double frac;
   long exp;
   double shifted_frac;
   double new_frac;
   double temp;
   real ret_val;

   shifter = pow((double) 10, (double) REAL_DIG - 2);
   frac = frexp((double) val, &exp);
   shifted_frac = frac * shifter;
   temp = modf(shifted_frac, &new_frac);
   new_frac /= shifter;
   ret_val = (real) ldexp(new_frac, exp);
   return ret_val;
}


void
PrintComplexNum (complex *c)
{
   if (c->i >= (real) 0.0)
      printf("%e + %ei", c->r, c->i);
   else
      printf("%e - %ei", c->r, -c->i);
}


void
PrintVector (vector *v)
{
   printf("(%10.5f, %10.5f)", v->x, v->y);
}


void
LockedPrint (char *format_str, ...)
{
   va_list ap;

   va_start(ap, format_str);
   {;};
   fflush(stdout);
   vfprintf(stdout, format_str, ap);
   fflush(stdout);
   {;};
   va_end(ap);
}


