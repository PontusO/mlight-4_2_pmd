


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
    /* Hog one timer for this thread */
    demo->timer = alloc_timer();
    demo->curr_P1_5 = P1_5;
    demo->state = DEMO_STATE_DAY;
    call_ramp (DEMO_DAYTIME, 1, 100, 100); //Starta med DAYTIME i full belysning
  //  call_ramp(0,1,100,0);
  //  call_ramp(1,1,100,0);
  //  call_ramp(2,1,100,0);
  //  call_ramp(3,1,100,0);


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
    PT_WAIT_UNTIL (&demo->pt, P1_5 == 1);

    if (demo->state == DEMO_STATE_DAY){

      /* P�b�rja en 10 sekunders uprampning av SUNSET och
       * n�r den kommit halvv�gs skall sl�ckningen av
       * DAYTIME p�b�rjas Sl�ckningen av DAYTIME skall ta 5 sekunder
       */
      call_ramp (DEMO_SUNSET, 10, 1, 50); // T�nd SUNSET till h�lften under 5 sekunder
      set_timer(demo->timer, 500, NULL);  // V�nta 5 sekunder (s� l�ng tid som upprampningen tar)
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);
      call_ramp(DEMO_DAYTIME, 5, 1, 0); // sl�ck DAYTIME
      call_ramp(DEMO_SUNSET, 10, 1, 100);  // t�nd SUNSET fullt

      set_timer(demo->timer, 500, NULL);  // V�nta 5 sekunder (s� l�ng tid som upp och nedrampningen tar
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);

      /* V�nta 5 sekunder, p�b�rja sen nedsl�ckning av SUNSET under 10 sek.
       * N�r SUNSET �r sl�ckt, t�nd NIGHT under 10 sekunder.
       * V�nta 10 sekunder, avsluta d�refter sekvensen
       */
      set_timer(demo->timer, 500, NULL);  // V�nta 5 sekunder
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);

      call_ramp(DEMO_SUNSET, 10, 1, 0);
      set_timer(demo->timer, 1000, NULL);  // V�nta 10 sekunder tills nedrampningen �r f�rdig
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);

      call_ramp(DEMO_NIGHT, 10, 1, 100); // T�nd NIGHT till max under 10 sekunder
      set_timer(demo->timer, 1000, NULL);  // V�nta 10 sekunder tills upprampningen �r f�rdig
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);


      demo->state = DEMO_STATE_NIGHT;  //avsluta sekvensen
    } else {
      /* G� fr�n natt till dag.
       * B�rja med att t�nda SUNRISE till h�lften under 5 sekunder.
       * N�r 5 sekunder g�tt, p�b�rja en 5 sekunders sl�ckning av NIGHT och
       * t�nd samtidigt upp SUNRISE till 100%.
       */
      call_ramp(DEMO_SUNRISE, 10, 1, 50); // p�b�ra t�ndning av SUNRISE
      set_timer(demo->timer, 500, NULL);  // V�nta 5 sekunder
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);
      call_ramp(DEMO_NIGHT, 10, 2, 0);    // Sl�ck ner NIGHT
      call_ramp(DEMO_SUNRISE, 10, 1, 100); //t�nd SUNRISE till fullt
      set_timer(demo->timer, 500, NULL);  // V�nta 5 sekunder tills SUNRISE �r t�nd
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);

      /* V�nta 5 sekunder, p�b�rja sedan t�ndning av DAYTIME till 50% under 5 sekunder.
       * N�r DAYTIME n�tt 50%, p�b�rja sl�ckningen av SUNRISE och t�nd
       * samtidigt DAATIME till 100% under 5 sekunder.
       */
      set_timer(demo->timer, 500, NULL);  // V�nta 5 sekunder
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);
      call_ramp(DEMO_DAYTIME, 10, 1, 50); // p�b�rja t�ndning av DAYTIME
      set_timer(demo->timer, 500, NULL);  // V�nta 5 sekunder
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);
      call_ramp(DEMO_SUNRISE, 10, 2, 0); //Sl�ck ner SUNrISE p� 5 sekunder
      call_ramp(DEMO_DAYTIME, 10, 1, 100); // t�nd DYATIME fullt p� 5 sekunder)

      set_timer(demo->timer, 1000, NULL);  // V�nta 10 sekunder
      PT_WAIT_UNTIL(&demo->pt, get_timer(demo->timer) == 0);

      demo->state = DEMO_STATE_DAY;
    }

  }

  PT_END(&demo->pt);
}


/* EOF */
