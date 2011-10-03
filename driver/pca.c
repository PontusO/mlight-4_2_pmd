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

#include "system.h"
#include "pca.h"
#include <iet_debug.h>

__xdata static unsigned char pcamode;
__xdata static u16_t values[4];

#define PCA_INT_ON()  EIE1 |= 0x08;
#define PCA_INT_OFF() EIE1 &= ~0x08;

void init_pca(unsigned char mode, unsigned char clock)
{
  unsigned char tmp = 0x4b;

  pcamode = mode;

  tmp |= mode;

  SFRPAGE   = PCA0_PAGE;
  PCA0CN    = 0x40;
  PCA0MD    = clock << PCA_CLK_SHIFT;
  PCA0CPM0  = tmp;
  PCA0CPM1  = tmp;
  PCA0CPM2  = tmp;
  PCA0CPM3  = tmp;

  /* Enable PCA interrupts */
  PCA_INT_ON();
}

char set_pca_duty (unsigned char channel, unsigned int duty)
{
  if (channel >= PCA_MAX_CHANNELS)
    return -1;

  if (pcamode & PCA_MODE_PWM_16) {
    PCA_INT_OFF();
    values[channel] = duty;
    PCA_INT_ON();
  } else {
    SFRPAGE = PCA0_PAGE;
    switch (channel)
    {
      case 0:
        PCA0CPH0 = duty;
        break;

      case 1:
        PCA0CPH1 = duty;
        break;

      case 2:
        PCA0CPH2 = duty;
        break;

      case 3:
        PCA0CPH3 = duty;
        break;

      default:
        return -1;
    }
  }

  return 0;
}

void PCA_ISR (void) __interrupt PCA_VECTOR __using 2
{
  /* Done in accordance with the data sheet */
  EA = 0;
  if (PCA0CN & 0x01) {
    PCA0CP0 = values[0];
    CCF0 = 0;
  }
  if (PCA0CN & 0x02) {
    PCA0CP1 = values[1];
    CCF1 = 0;
  }
  if (PCA0CN & 0x04) {
    PCA0CP2 = values[2];
    CCF2 = 0;
  }
  if (PCA0CN & 0x08) {
    PCA0CP3 = values[3];
    CCF3 = 0;
  }
  EA = 1;
}
