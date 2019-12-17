#line 124 "/home/qiang/src/splash2/splash2-master/codes/null_macros/c.m4.null.POSIX_sel4"

#line 1 "mdmain.C"
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

#include <autoconf.h>
#include <manager/gen_config.h>
#include <side-bench/gen_config.h>
#line 17
#include <unistd.h>
#line 17
#include <stdlib.h>
#line 17
#include <malloc.h>
#line 17
#include <sel4bench/sel4bench.h>
#line 17

#include "stdio.h"
#include "parameters.h"
#include "mdvar.h"
#include "water.h"
#include "wwpot.h"
#include "cnst.h"
#include "mddata.h"
#include "fileio.h"
#include "split.h"
#include "global.h"

/************************************************************************/

/* routine that implements the time-steps. Called by main routine and calls others */
double MDMAIN(long NSTEP, long NPRINT, long NSAVE, long NORD1, long ProcID)
{
    double XTT;
    long i;
    double POTA,POTR,POTRF;
    double XVIR,AVGT,TEN;
    double TTMV = 0.0, TKIN = 0.0, TVIR = 0.0;

    /*.......ESTIMATE ACCELERATION FROM F/M */
    INTRAF(&gl->VIR,ProcID);

    {
#line 43
	;
#line 43
};

    INTERF(ACC,&gl->VIR,ProcID);

    {
#line 47
	;
#line 47
};

    /* MOLECULAR DYNAMICS LOOP OVER ALL TIME-STEPS */

    for (i=1;i <= NSTEP; i++) {
        TTMV=TTMV+1.00;

        /* reset simulator stats at beginning of second time-step */

        /* POSSIBLE ENHANCEMENT:  Here's where one start measurements to avoid
           cold-start effects.  Recommended to do this at the beginning of the
           second timestep; i.e. if (i == 2).
           */

        /* initialize various shared sums */
        if (ProcID == 0) {
            long dir;
            if (i >= 2) {
                {
#line 65
	(gl->trackstart) = sel4bench_get_cycle_count();
#line 65
};
            }
            gl->VIR = 0.0;
            gl->POTA = 0.0;
            gl->POTR = 0.0;
            gl->POTRF = 0.0;
            for (dir = XDIR; dir <= ZDIR; dir++)
                gl->SUM[dir] = 0.0;
        }

        if ((ProcID == 0) && (i >= 2)) {
            {
#line 76
	(gl->intrastart) = sel4bench_get_cycle_count();
#line 76
};
        }

        {
#line 79
	;
#line 79
};
        PREDIC(TLC,NORD1,ProcID);
        INTRAF(&gl->VIR,ProcID);
        {
#line 82
	;
#line 82
};

        if ((ProcID == 0) && (i >= 2)) {
            {
#line 85
	(gl->intraend) = sel4bench_get_cycle_count();
#line 85
};
            gl->intratime += gl->intraend - gl->intrastart;
        }


        if ((ProcID == 0) && (i >= 2)) {
            {
#line 91
	(gl->interstart) = sel4bench_get_cycle_count();
#line 91
};
        }

        INTERF(FORCES,&gl->VIR,ProcID);

        if ((ProcID == 0) && (i >= 2)) {
            {
#line 97
	(gl->interend) = sel4bench_get_cycle_count();
#line 97
};
            gl->intertime += gl->interend - gl->interstart;
        }

        if ((ProcID == 0) && (i >= 2)) {
            {
#line 102
	(gl->intrastart) = sel4bench_get_cycle_count();
#line 102
};
        }

        CORREC(PCC,NORD1,ProcID);

        BNDRY(ProcID);

        KINETI(gl->SUM,HMAS,OMAS,ProcID);

        {
#line 111
	;
#line 111
};

        if ((ProcID == 0) && (i >= 2)) {
            {
#line 114
	(gl->intraend) = sel4bench_get_cycle_count();
#line 114
};
            gl->intratime += gl->intraend - gl->intrastart;
        }

        TKIN=TKIN+gl->SUM[0]+gl->SUM[1]+gl->SUM[2];
        TVIR=TVIR-gl->VIR;

        /*  check if potential energy is to be computed, and if
            printing and/or saving is to be done, this time step.
            Note that potential energy is computed once every NPRINT
            time-steps */

        if (((i % NPRINT) == 0) || ( (NSAVE > 0) && ((i % NSAVE) == 0))){

            if ((ProcID == 0) && (i >= 2)) {
                {
#line 129
	(gl->interstart) = sel4bench_get_cycle_count();
#line 129
};
            }

            /*  call potential energy computing routine */
            POTENG(&gl->POTA,&gl->POTR,&gl->POTRF,ProcID);
            {
#line 134
	;
#line 134
};

            if ((ProcID == 0) && (i >= 2)) {
                {
#line 137
	(gl->interend) = sel4bench_get_cycle_count();
#line 137
};
                gl->intertime += gl->interend - gl->interstart;
            }

            POTA=gl->POTA*FPOT;
            POTR=gl->POTR*FPOT;
            POTRF=gl->POTRF*FPOT;

            /* compute some values to print */
            XVIR=TVIR*FPOT*0.50/TTMV;
            AVGT=TKIN*FKIN*TEMP*2.00/(3.00*TTMV);
            TEN=(gl->SUM[0]+gl->SUM[1]+gl->SUM[2])*FKIN;
            XTT=POTA+POTR+POTRF+TEN;

            if ((i % NPRINT) == 0 && ProcID == 0) {
#ifdef CONFIG_DEBUG_BUILD
  
                fprintf(six,"     %5ld %14.5lf %12.5lf %12.5lf  \
                %12.5lf\n %16.3lf %16.5lf %16.5lf\n",
                        i,TEN,POTA,POTR,POTRF,XTT,AVGT,XVIR);
#endif 
            }
        }

        /* wait for everyone to finish time-step */
        {
#line 159
	;
#line 159
};

        if ((ProcID == 0) && (i >= 2)) {
            {
#line 162
	(gl->trackend) = sel4bench_get_cycle_count();
#line 162
};
            gl->tracktime += gl->trackend - gl->trackstart;
        }
    } /* for i */

    return(XTT);

} /* end of subroutine MDMAIN */
