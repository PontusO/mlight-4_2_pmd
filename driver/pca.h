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
#ifndef PCA_H_INCLUDED
#define PCA_H_INCLUDED

#define PCA_MAX_CHANNELS  4

#define PCA_CHANNEL0      0
#define PCA_CHANNEL1      1
#define PCA_CHANNEL2      2
#define PCA_CHANNEL3      3

#define PCA_MODE_PCAP     1
#define PCA_MODE_NCAP     2
#define PCA_MODE_BCAP     3
#define PCA_MODE_PWM_8    0x00
#define PCA_MODE_PWM_16   0x80

#define PCA_SYS_CLK_DIV12 0
#define PCA_SYS_CLK_DIV4  1
#define PCA_TIMER0_OVFL   2
#define PCA_ECI_HI_TO_LOW 3
#define PCA_SYS_CLK       4
#define PCA_EXT_CLK_DIV8  5
#define PCA_CLK_SHIFT     1

void init_pca(unsigned char mode, unsigned char clock);
char set_pca_duty (unsigned char channel, unsigned int duty);

#endif // PCA_H_INCLUDED
