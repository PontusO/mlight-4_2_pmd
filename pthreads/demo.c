


/*
 * Copyright (c) 2011, Pontus Oldberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#pragma codeseg APP_BANK

#define PRINT_A     // Enable A prints

#include "system.h"
#include "iet_debug.h"
#include "demo.h"
#include "pca.h"
#include "string.h"
#include "stdlib.h"
#include "swtimers.h"
#include <but_mon.h>
#include "lightlib.h"
#include "ramp_mgr.h"
/*
 * Locally used data
 */

/*
 * Initialize the ramp manager
 */
void init_demo(demo_t *demo) __reentrant banked
{

    memset (demo, 0, sizeof *demo);
    PT_INIT(&demo->pt);
    demo->curr_P1_5 = P1_5;
    demo->state = DEMO_STATE_DAY;
    call_ramp (DEMO_DAYTIME, 1, 2, 100); //Starta med DAYTIME i full belysning


}

void call_ramp(u8_t channel, u8_t rate, u8_t step, u8_t rampto) __reentrant banked
{
  ramp_mgr_t *rmptr = get_ramp_mgr (channel);
      /* Assert a signal to the ramp manager to start a ramp */
      A_(printf (__FILE__ " Starting ramp (%p) on channel %d\n",
                 rmptr, (int)channel);)
      rmptr->rate = rate;
      rmptr->step = step;
      rmptr->rampto = rampto;
      rmptr->signal = RAMP_CMD_START;

}

void terminate_ramp(u8_t channel) __reentrant banked
{
  ramp_mgr_t *rmptr = get_ramp_mgr (channel);
  rmptr->ramp.signal = RAMP_CMD_STOP;

}

PT_THREAD(handle_demo(demo_t *demo) __reentrant banked)
{


  PT_BEGIN(&demo->pt);

  A_(printf (__FILE__ " Starting demo\n");)

  while (1)
  {
       /* Wait for a button to be pressed */
    PT_WAIT_UNTIL (&demo->pt, P1_5 == 1 && demo->state != DEMO_ONGOING);
    demo->state = DEMO_ONGOING;

    if (demo->state == DEMO_STATE_DAY){
      A_(printf (__FILE__ "DAYTIME to 0\n");)

      call_ramp (DEMO_DAYTIME, 1, 10, 0);

      A_(printf (__FILE__ " NIGHT up to 100\n");)
      call_ramp(DEMO_NIGHT, 1, 10, 100);


      demo->state = DEMO_STATE_NIGHT;  //avsluta sekvensen
    } else {
      A_(printf (__FILE__ " Night to 0\n");)
      call_ramp(DEMO_NIGHT, 1, 10, 0);
       A_(printf (__FILE__ " DAYTIME up 100\n");)
      call_ramp(DEMO_DAYTIME, 1, 10, 100);

      demo->state = DEMO_STATE_DAY;
    }

  }

  PT_END(&demo->pt);
}


/* EOF */
