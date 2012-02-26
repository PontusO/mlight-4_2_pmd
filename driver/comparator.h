/*
 * Copyright (c) 2012, Pontus Oldberg.
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
#ifndef COMPARATOR_H_INCLUDED
#define COMPARATOR_H_INCLUDED

#define COMP_NUM_COMPARATORS  0x02

/* Control registers default values */
#define CPTXMD_DEFAULT_VALUE  0x02
#define CPTXCN_DEFAULT_VALUE  0x00

#define COMP_ENABLE           0x80
#define COMP_OUT              0x40
#define COMP_RISE_EDGE        0x20
#define COMP_FALL_EDGE        0x10
#define COMP_POSHYS_DIS       0x00
#define COMP_POSHYS_5MV       0x04
#define COMP_POSHYS_10MV      0x08
#define COMP_POSHYS_15MV      0x0c
#define COMP_NEGHYS_DIS       0x00
#define COMP_NEGHYS_5MV       0x01
#define COMP_NEGHYS_10MV      0x02
#define COMP_NEGHYS_15MV      0x03

enum comparator_status_t {
  COMP_FREE = 0x00,
  COMP_ALLOCATED  = 0x01,
};

enum comparator_errors_t {
  COMP_ERR_OK = 0x00,
  COMP_ERR_ALREADY_IN_USE = 0x01,
  COMP_ERR_NO_SUCH_DEVICE = 0x02,
};

void init_comparators (void);
char allocate_comparator (u8_t comp);
char free_comparator (u8_t comp);
char get_comparator_state (u8_t comp);

#endif // COMPARATOR_H_INCLUDED
