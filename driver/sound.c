#pragma codeseg  APP_BANK
/*
 * Copyright (c) 2008, Pontus Oldberg.
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

#include "system.h"
#include "sound.h"
#include "swtimers.h"

/*
 * Local data
 */
struct sound sys_snd;

/*
 * This function sets up any hw and or parameters needed to operate
 * the sound channel.
 */
void init_sound(void) banked
{
  /* Not sure what to do here yet =), but we can at least make sure that
   * timer4 is shut down and the protothread is initialized.
   */
  SFRPAGE   = TMR4_PAGE;
  TMR4CN = 0x00;
  SFRPAGE = LEGACY_PAGE;

  PT_INIT(&sys_snd.sound_pt);
}

/*
 * This function outputs a beep with a fixed frequency and time on
 * the sound channel.
 */
void beep(u16_t freq, u16_t time) banked
{
  /* Set the sound struct up to play a system sound */
  sys_snd.timeout = time;
  sys_snd.frequency = FREQUENCY(freq);
  sys_snd.command = PLAY_SOUND;   /* Start playing */
}
/*
 * This protothread takes care of playing sounds on the system speaker
 */
PT_THREAD(handle_sound(struct sound *snd) banked)
{
  PT_BEGIN(&snd->sound_pt);

  /* Yes, we are hogging the timer */
  snd->timer = alloc_timer();

  while (1)
  {
    /* Wait for a sound command to come */
    PT_WAIT_UNTIL(&snd->sound_pt, snd->command);
    snd->command = 0x00;

    SFRPAGE   = TMR4_PAGE;
    /* Set Timer4 Configuration
     * T4M1(4)    = 1
     * T4M0(3)    = 1
     * TOG4(2)    = 0
     * T4OE(1)    = 1
     * DCEN4(0)   = 0
     */
    TMR4CF = 0x1A;

    /* Set the frequency */
    RCAP4H = snd->frequency / 256;
    RCAP4L = snd->frequency & 0xff;

    /* Set Timer4 Control
     * TF(7)      = 0
     * EXF4(6)    = 0
     * EXEN4(3)   = 0
     * TR4(2)     = 1
     * C/T4(1)    = 0
     * CP/RL(0)   = 0
     */
    TMR4CN = 0x04;
    SFRPAGE = LEGACY_PAGE;

    set_timer(snd->timer, snd->timeout, NULL);

    /* Wait for the duration timer to time out */
    PT_WAIT_UNTIL(&snd->sound_pt, (get_timer(snd->timer) == 0x0000));

    /* Shut off sound */
    SFRPAGE   = TMR4_PAGE;
    TMR4CN = 0x00;
  }

  PT_END(&snd->sound_pt);
}


