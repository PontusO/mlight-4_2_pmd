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
#pragma codeseg  APP_BANK
#pragma constseg APP_BANK

#include <string.h>

#include "system.h"
#include "pt.h"
#include "remote.h"
#include "sound.h"
#include "iet_debug.h"
#include "led.h"
#include "flash.h"
#include "swtimers.h"

/* Local variables and storage */
unsigned char remote_state;
xdata u16_t time_array[256];
xdata u16_t average[256];
unsigned char array_ptr;
unsigned char length;
bit SIG_REMOTE_PROCESS;             // Trigger to process incoming data
bit ENABLE_REMOTE_TRAINER_MODE;
char key_code;
char last_key;
static u8_t i, j;

#define SAVE_PSBANK(n)      n = PSBANK
#define SET_PSBANK(bank)    PSBANK = (PSBANK & 0xcf) | (bank << 4)
#define RESTORE_PSBANK(n)   PSBANK = n

#define REMOTE_DORMANT      1
#define REMOTE_STARTBIT     2
#define REMOTE_MEASURING    3

#define RUN                 1
#define STOP                0

/* Remote control codes are stored in APP_BANK, must be an even 1024 multiple */
/* Important notice: As it is impossible to erase the block located at F800 and the next block
 * is reserved by the MCU we need to leave these areas alone. More in the datasheet at section:
 * 15.2 about that
 */
#define RC_STORAGE          0xE000
#define MAX_RC_STORE        0xF800
#define MAX_KEY             34
#define MAX_CODE_ENTRIES    63
#define AVERAGE_LENGTH      5

struct cremote cremote;
struct remote_trainer rtm;

/*
 * Predefined RC5 codes
 */
static const u16_t code_0[]       = {0x001a, 0x0649, 0x0bb8, 0x11e3, 0x1753, 0x234b, 0x28c3, 0x2ee8, 0x345c,
                                     0x3a7f, 0x3fee, 0x4619, 0x4b88, 0x51b3, 0x5722, 0x5d4d, 0x62bc, 0x68e7,
                                     0x6e56, 0x7481, 0x79f0, 0x801b, 0x858a, 0x8bb5, 0x9124, 0x974e, };
static const u16_t code_0a[]      = {0x001a, 0x0647, 0x0bb7, 0x17af, 0x1d00, 0x2349, 0x28b8, 0x2ee3, 0x3452,
                                     0x3a7d, 0x3fec, 0x4617, 0x4b86, 0x51b1, 0x5720, 0x5d4b, 0x62ba, 0x68e5,
                                     0x6e54, 0x747f, 0x79ee, 0x8019, 0x8587, 0x8bb3, 0x9122, 0x974d, };
static const u16_t code_1[]       = {0x001a, 0x064c, 0x0bba, 0x11e6, 0x1754, 0x234d, 0x28c3, 0x2ee7, 0x3456,
                                     0x3a81, 0x3ff0, 0x461b, 0x4b8a, 0x51b5, 0x5724, 0x5d4f, 0x62be, 0x68e9,
                                     0x6e57, 0x7483, 0x79f1, 0x801d, 0x858b, 0x8bb7, 0x96fa, 0x9d1d, };
static const u16_t code_1a[]      = {0x001a, 0x064a, 0x0bba, 0x17b2, 0x1d29, 0x234c, 0x28ba, 0x2ee5, 0x3454,
                                     0x3a7f, 0x3fee, 0x4619, 0x4b88, 0x51b3, 0x5723, 0x5d4d, 0x62bc, 0x68e7,
                                     0x6e56, 0x7481, 0x79f0, 0x801b, 0x858b, 0x8bb6, 0x96f8, 0x9d1c, };
static const u16_t code_2[]       = {0x0018, 0x064b, 0x0bba, 0x11e6, 0x1755, 0x234c, 0x28c5, 0x2ee8, 0x345d,
                                     0x3a80, 0x3fef, 0x461a, 0x4b89, 0x51b4, 0x5723, 0x5d4e, 0x62bd, 0x68e8,
                                     0x6e57, 0x7482, 0x79f1, 0x801c, 0x8b62, 0x9753, };
static const u16_t code_2a[]      = {0x0018, 0x064b, 0x0bb9, 0x17b2, 0x1d28, 0x234b, 0x28bb, 0x2ee5, 0x3454,
                                     0x3a80, 0x3fee, 0x4619, 0x4b88, 0x51b3, 0x5723, 0x5d4e, 0x62bc, 0x68e8,
                                     0x6e57, 0x7481, 0x79f1, 0x801c, 0x8b60, 0x9750, };
static const u16_t code_3[]       = {0x001a, 0x0648, 0x0bb8, 0x11e2, 0x1751, 0x234a, 0x28c3, 0x2ee6, 0x345a,
                                     0x3a7d, 0x3fec, 0x4617, 0x4b86, 0x51b1, 0x5721, 0x5d4b, 0x62bb, 0x68e5,
                                     0x6e55, 0x747f, 0x79ef, 0x8019, 0x8b5f, 0x9184, 0x96f7, 0x9d1b, };
static const u16_t code_3a[]      = {0x001a, 0x064a, 0x0bb9, 0x17b2, 0x1d2b, 0x234e, 0x28c2, 0x2ee6, 0x3455,
                                     0x3a80, 0x3fef, 0x461a, 0x4b89, 0x51b4, 0x5722, 0x5d4d, 0x62bc, 0x68e8,
                                     0x6e57, 0x7481, 0x79f1, 0x801c, 0x8b61, 0x9186, 0x96f8, 0x9d1d, };
static const u16_t code_4[]       = {0x0018, 0x0649, 0x0bb9, 0x11e4, 0x1753, 0x234b, 0x28c4, 0x2ee8, 0x345c,
                                     0x3a7f, 0x3fee, 0x4619, 0x4b88, 0x51b3, 0x5722, 0x5d4d, 0x62bc, 0x68e7,
                                     0x6e56, 0x7481, 0x7fc7, 0x8bb9, 0x912c, 0x974f, };
static const u16_t code_4a[]      = {0x0018, 0x064b, 0x0bbb, 0x17b3, 0x1d2c, 0x2350, 0x28c4, 0x2ee6, 0x3456,
                                     0x3a80, 0x3ff1, 0x461b, 0x4b8b, 0x51b5, 0x5724, 0x5d4f, 0x62bf, 0x68e9,
                                     0x6e59, 0x7482, 0x7fc6, 0x8bb7, 0x9130, 0x9754, };
static const u16_t code_5[]       = {0x0018, 0x064a, 0x0bb9, 0x11e4, 0x1753, 0x234b, 0x28c1, 0x2ee5, 0x3454,
                                     0x3a7f, 0x3fef, 0x4619, 0x4b88, 0x51b3, 0x5722, 0x5d4d, 0x62bc, 0x68e7,
                                     0x6e56, 0x7481, 0x7fc4, 0x8bb5, 0x96f9, 0x9d1c, };
static const u16_t code_5a[]      = {0x0018, 0x064a, 0x0bba, 0x17b1, 0x1d2a, 0x234e, 0x28c2, 0x2ee6, 0x3455,
                                     0x3a7f, 0x3fef, 0x461a, 0x4b89, 0x51b4, 0x5723, 0x5d4e, 0x62bd, 0x68e8,
                                     0x6e57, 0x7482, 0x7fc8, 0x8bb9, 0x96fb, 0x9d20, };
static const u16_t code_6[]       = {0x0018, 0x064b, 0x0bba, 0x17b2, 0x1d2b, 0x2350, 0x289b, 0x2ee6, 0x3456,
                                     0x3a81, 0x3ff0, 0x461a, 0x4b8a, 0x51b4, 0x5724, 0x5d4e, 0x62be, 0x68e9,
                                     0x6e58, 0x7482, 0x7fc8, 0x85ed, 0x8b60, 0x9750, };
static const u16_t code_6a[]      = {0x0018, 0x0647, 0x0bb7, 0x11e1, 0x1751, 0x2349, 0x28c1, 0x2ee6, 0x3459,
                                     0x3a7d, 0x3fec, 0x4617, 0x4b86, 0x51b0, 0x5720, 0x5d4a, 0x62ba, 0x68e5,
                                     0x6e53, 0x747e, 0x7fc4, 0x85e8, 0x8b5c, 0x974d, };
static const u16_t code_7[]       = {0x001a, 0x0649, 0x0bb9, 0x17b1, 0x1d2a, 0x234e, 0x28c2, 0x2ee5, 0x3455,
                                     0x3a7f, 0x3fee, 0x4619, 0x4b88, 0x51b2, 0x5722, 0x5d4d, 0x62bc, 0x68e6,
                                     0x6e56, 0x7480, 0x7fc6, 0x85ea, 0x8b5f, 0x9181, 0x96f1, 0x9d1c, };
static const u16_t code_7a[]      = {0x001a, 0x064a, 0x0bba, 0x11e4, 0x1754, 0x234b, 0x28c3, 0x2ee5, 0x3455,
                                     0x3a7f, 0x3fef, 0x4619, 0x4b88, 0x51b3, 0x5723, 0x5d4d, 0x62bc, 0x68e7,
                                     0x6e57, 0x7481, 0x7fc5, 0x85e8, 0x8b57, 0x9182, 0x96f2, 0x9d1c, };
static const u16_t code_8[]       = {0x0018, 0x0648, 0x0bb8, 0x17af, 0x1d26, 0x234a, 0x28b8, 0x2ee3, 0x3452,
                                     0x3a7d, 0x3fec, 0x4617, 0x4b86, 0x51b1, 0x5720, 0x5d4c, 0x62bb, 0x68e5,
                                     0x7429, 0x801a, 0x856b, 0x8bb7, 0x912a, 0x974d, };
static const u16_t code_8a[]      = {0x0018, 0x064b, 0x0bba, 0x11e5, 0x1754, 0x234d, 0x28c6, 0x2eea, 0x345d,
                                     0x3a81, 0x3ff0, 0x461b, 0x4b8a, 0x51b5, 0x5724, 0x5d4f, 0x62be, 0x68e9,
                                     0x742e, 0x801f, 0x8594, 0x8bb7, 0x9125, 0x9750, };
static const u16_t code_9[]       = {0x0018, 0x0649, 0x0bb9, 0x11e3, 0x1753, 0x234b, 0x28c2, 0x2ee8, 0x345b,
                                     0x3a7f, 0x3fee, 0x4619, 0x4b88, 0x51b3, 0x5722, 0x5d4d, 0x62bc, 0x68e7,
                                     0x742a, 0x801b, 0x8594, 0x8bb8, 0x96fb, 0x9d1f, };
static const u16_t code_9a[]      = {0x0018, 0x064b, 0x0bbb, 0x17b3, 0x1d2b, 0x234f, 0x28c3, 0x2ee6, 0x3456,
                                     0x3a80, 0x3ff0, 0x461a, 0x4b8a, 0x51b4, 0x5723, 0x5d4e, 0x62be, 0x68e8,
                                     0x742e, 0x8020, 0x8593, 0x8bb6, 0x96fc, 0x9d21, };
static const u16_t code_red[]     = {0x0018, 0x064d, 0x0bbb, 0x11e7, 0x1756, 0x234e, 0x28c7, 0x2eeb, 0x345e,
                                     0x3a82, 0x3ff0, 0x461c, 0x4b8a, 0x51b6, 0x5cf9, 0x631d, 0x688b, 0x7484,
                                     0x7fc9, 0x85ee, 0x8b61, 0x9185, 0x96f3, 0x9d1f, };
static const u16_t code_reda[]    = {0x0018, 0x0648, 0x0bb7, 0x17af, 0x1d28, 0x234c, 0x28c0, 0x2ee3, 0x3453,
                                     0x3a7d, 0x3fed, 0x4617, 0x4b86, 0x51b2, 0x5cf5, 0x6318, 0x6888, 0x747f,
                                     0x7fc5, 0x85e9, 0x8b5d, 0x9180, 0x96f0, 0x9d1a, };
static const u16_t code_green[]   = {0x0016, 0x061c, 0x0bb9, 0x17b1, 0x1d29, 0x2326, 0x28c1, 0x2ee5, 0x3454,
                                     0x3a7f, 0x3fed, 0x461a, 0x4b88, 0x51b4, 0x5cf6, 0x6318, 0x6888, 0x7481,
                                     0x7fc6, 0x85bf, 0x8b5d, 0x974e, };
static const u16_t code_greena[]  = {0x0016, 0x064b, 0x0bba, 0x11e5, 0x1753, 0x234c, 0x28c5, 0x2ee8, 0x345b,
                                     0x3a80, 0x3fee, 0x461a, 0x4b88, 0x51b3, 0x5cf6, 0x631b, 0x6889, 0x7482,
                                     0x7fc5, 0x85e8, 0x8b57, 0x9750, };
static const u16_t code_yellow[]  = {0x0016, 0x064b, 0x0bba, 0x17b2, 0x1d2b, 0x234f, 0x28c2, 0x2ee6, 0x3455,
                                     0x3a80, 0x3fef, 0x4619, 0x4b89, 0x51b3, 0x5cf8, 0x631e, 0x6891, 0x7482,
                                     0x79f9, 0x801c, 0x8b61, 0x9753, };
static const u16_t code_yellowa[] = {0x0016, 0x064b, 0x0bba, 0x11e5, 0x1754, 0x234c, 0x28c5, 0x2ee9, 0x345d,
                                     0x3a80, 0x3fef, 0x461a, 0x4b89, 0x51b4, 0x5cf6, 0x631b, 0x688a, 0x7482,
                                     0x79fb, 0x801f, 0x8b5f, 0x9750, };
static const u16_t code_blue[]    = {0x0016, 0x064a, 0x0bb9, 0x17b2, 0x1d04, 0x234e, 0x28c2, 0x2ee5, 0x3455,
                                     0x3a7f, 0x3fee, 0x4619, 0x4b89, 0x51b4, 0x5cf8, 0x631d, 0x6890, 0x7481,
                                     0x7fc7, 0x8bb8, 0x912b, 0x974f, };
static const u16_t code_bluea[]   = {0x0016, 0x064a, 0x0bb9, 0x11e3, 0x1753, 0x234b, 0x28c4, 0x2ee8, 0x345b,
                                     0x3a7e, 0x3fee, 0x4618, 0x4b88, 0x51b2, 0x5cf5, 0x631a, 0x6888, 0x7481,
                                     0x7fc4, 0x8bb5, 0x912d, 0x9751, };
static const u16_t code_up[]      = {0x0018, 0x064a, 0x0bb9, 0x17b1, 0x1d2a, 0x234e, 0x28c2, 0x2ee5, 0x3455,
                                     0x3a7f, 0x3fee, 0x4619, 0x4b88, 0x51b3, 0x5cd0, 0x68e7, 0x6e60, 0x7484,
                                     0x79f7, 0x801b, 0x858a, 0x8bb5, 0x9124, 0x974f, };
static const u16_t code_upa[]     = {0x0018, 0x0648, 0x0bb7, 0x11e2, 0x1751, 0x2349, 0x28c2, 0x2ee6, 0x3459,
                                     0x3a7d, 0x3fec, 0x4617, 0x4b86, 0x51b1, 0x5cf4, 0x68e5, 0x6e5d, 0x7483,
                                     0x79f5, 0x8019, 0x8589, 0x8bb3, 0x9122, 0x974d, };
static const u16_t code_dn[]      = {0x0018, 0x064a, 0x0bba, 0x17b2, 0x1d2a, 0x234f, 0x28c2, 0x2ee6, 0x3454,
                                     0x3a80, 0x3fee, 0x461a, 0x4b88, 0x51b3, 0x5cf7, 0x68e8, 0x6e60, 0x7484,
                                     0x79f7, 0x801b, 0x858a, 0x8bb5, 0x96fa, 0x9d1f, };
static const u16_t code_dna[]     = {0x0018, 0x0648, 0x0bb7, 0x11e3, 0x1751, 0x234a, 0x28bf, 0x2ee4, 0x3452,
                                     0x3a7d, 0x3fec, 0x4618, 0x4b86, 0x51b2, 0x5cf7, 0x68e9, 0x6e5e, 0x7483,
                                     0x79f6, 0x801a, 0x8588, 0x8bb4, 0x96f6, 0x9d1b, };
static const u16_t code_pwr[]     = {0x0018, 0x064c, 0x0bbc, 0x17b4, 0x1d2a, 0x234e, 0x28bd, 0x2ee8, 0x3457,
                                     0x3a82, 0x3ff0, 0x461c, 0x4b8b, 0x51b6, 0x5725, 0x5d50, 0x62bf, 0x68ea,
                                     0x7407, 0x7a54, 0x7fc8, 0x8bb8, 0x9131, 0x9755, };
static const u16_t code_pwra[]    = {0x0018, 0x064b, 0x0bba, 0x11e5, 0x1754, 0x234c, 0x28c5, 0x2ee9, 0x345d,
                                     0x3a80, 0x3fef, 0x461a, 0x4b89, 0x51b4, 0x5723, 0x5d4e, 0x62bd, 0x68e8,
                                     0x742b, 0x7a4f, 0x7fbe, 0x8bb7, 0x9130, 0x9754, };

/*
 * A convenient list of pointers to our remote codes
 */
u16_t *code_list[] = {
  &code_0[0],
  &code_0a[0],
  &code_1[0],
  &code_1a[0],
  &code_2[0],
  &code_2a[0],
  &code_3[0],
  &code_3a[0],
  &code_4[0],
  &code_4a[0],
  &code_5[0],
  &code_5a[0],
  &code_6[0],
  &code_6a[0],
  &code_7[0],
  &code_7a[0],
  &code_8[0],
  &code_8a[0],
  &code_9[0],
  &code_9a[0],
  &code_red[0],
  &code_reda[0],
  &code_green[0],
  &code_greena[0],
  &code_yellow[0],
  &code_yellowa[0],
  &code_blue[0],
  &code_bluea[0],
  &code_up[0],
  &code_upa[0],
  &code_dn[0],
  &code_dna[0],
  &code_pwr[0],
  &code_pwra[0],
  NULL
};

/*
 * Every key code also has an attribute connected to it. This tells wether the
 * key supports repeat, long press etc.
 */
enum KEY_ATTRIBUTES {
  KEY_LONG_PRESS = 0x80,  /* This option and KEY_REPEAT are mutually exclusive */
  KEY_REPEAT = 0x40       /* This option and KEY_LONG_PRESS are mutually exclusive */
};

static u8_t key_attributes[MAX_KEY] = {
  KEY_REPEAT,             /* Key 0 */
  KEY_REPEAT,             /* Key 0 */
  KEY_REPEAT,             /* Key 1 */
  KEY_REPEAT,             /* Key 1 */
  KEY_REPEAT,             /* Key 2 */
  KEY_REPEAT,             /* Key 2 */
  KEY_REPEAT,             /* Key 3 */
  KEY_REPEAT,             /* Key 3 */
  KEY_REPEAT,             /* Key 4 */
  KEY_REPEAT,             /* Key 4 */
  KEY_REPEAT,             /* Key 5 */
  KEY_REPEAT,             /* Key 5 */
  KEY_REPEAT,             /* Key 6 */
  KEY_REPEAT,             /* Key 6 */
  KEY_REPEAT,             /* Key 7 */
  KEY_REPEAT,             /* Key 7 */
  KEY_REPEAT,             /* Key 8 */
  KEY_REPEAT,             /* Key 8 */
  KEY_REPEAT,             /* Key 9 */
  KEY_REPEAT,             /* Key 9 */
  KEY_REPEAT,             /* Key red */
  KEY_REPEAT,             /* Key red */
  KEY_REPEAT,             /* Key green */
  KEY_REPEAT,             /* Key green */
  KEY_REPEAT,             /* Key yellow */
  KEY_REPEAT,             /* Key yellow */
  KEY_REPEAT,             /* Key blue */
  KEY_REPEAT,             /* Key blue */
  KEY_REPEAT,             /* Key up */
  KEY_REPEAT,             /* Key up */
  KEY_REPEAT,             /* Key dn */
  KEY_REPEAT,             /* Key dn */
  KEY_LONG_PRESS,         /* Key pwr */
  KEY_LONG_PRESS,         /* Key pwr */
};

//-----------------------------------------------------------------------------
// remote_get_packet_length();
//   Returns the length of the current bit packet in the buffer;
//-----------------------------------------------------------------------------
u8_t remote_get_packet_length(void) banked
{
  return length;
}

//-----------------------------------------------------------------------------
// remote_get_array(u8_t n);
//   Returns the specified entry in the time_array buffer;
//-----------------------------------------------------------------------------
u16_t remote_get_array(u8_t n) banked
{
  return time_array[n];
}


//-----------------------------------------------------------------------------
// PCA_Init - Initialize the PCA block
//-----------------------------------------------------------------------------
static void PCA_Init(void)
{
#if BUILD_TARGET == IET912X
  SFRPAGE   = PCA0_PAGE;
#endif
  /* Enable PCA0 module 0 interrupts and start the PCA counter */
  PCA0CN = 0x41;
  /* Select clock (SYSCLK/12) and disable PCA interrupts */
  PCA0MD = 0x00;
  /* Enable positive and negative edge capture and enable interrupts */
  PCA0CPM0 = 0x31;
#if BUILD_TARGET == IET912X
  SFRPAGE = LEGACY_PAGE;                // Reset to legacy SFR page
#endif
}

//-----------------------------------------------------------------------------
// Timer1_Init - Used to detect a bit timeout in the remote communication
//-----------------------------------------------------------------------------
static void Timer1_Init (int counts)
{
#if BUILD_TARGET == IET912X
  SFRPAGE = TIMER01_PAGE;               // Set the correct SFR page
#endif
  TF1 = 0;                              // Make sure no interrupts happen
  TR1 = STOP;	                          // Stop Timer1
  CKCON &= ~0x10;	                      // T1M set for SYSCLK/12
  TH1 = (-(counts) >> 8);	              // Set the requested value
  TL1 = 0xff;		                        // initialize Timer1
  TMOD |= 0x10;	                        // TMOD: timer 1, mode 1, 16-bit timer
#if BUILD_TARGET == IET912X
  SFRPAGE = LEGACY_PAGE;                // Reset to legacy SFR page
#endif
}

//-----------------------------------------------------------------------------
// Initialize all remote control resources needed
//-----------------------------------------------------------------------------
void remote_init(void) banked
{
  remote_state = REMOTE_DORMANT;
  array_ptr = 2;
  length = 0;

  /* Initialize last_key to any not used key code */
  last_key = -1;
  key_code = -1;

  /* Set up timer and PCA */
  Timer1_Init(0);
  PCA_Init();

  /* Enable Timer 1 interrupts */
  IE |= 0x08;
  /* Enable PCA interrupts */
  EIE1 |= 0x08;

  /* Trainer init */
  ENABLE_REMOTE_TRAINER_MODE = 0;
  rtm.i = 0;

  PT_INIT(&rtm.pt);
  PT_INIT(&cremote.pt);
}

/*
 * this function returns a code key if it exists, or -1 if there was no key code entry
 */
char get_remote_key(void) banked
{
  char temp = key_code;
  if (temp != -1)
    key_code = -1;
  return temp;
}

/**
 * Copy default rc codes to flash.
 */
void copy_rc_to_flash(void) banked
{
  u8_t i, tmp;
  u8_t *ptr;

  SAVE_PSBANK(tmp);
  SET_PSBANK(APP_BANK);

  for (i=0 ; i<MAX_KEY ; i++) {
    /* Go ahead and program flash */
    ptr = (u8_t*)code_list[i];
    write_to_flash((xdata u8_t*)(RC_STORAGE + i * 128),
                   (code u8_t*)code_list[i], (*((u16_t*)code_list[i])) * 2,
                   APP_BANK);
  }
  RESTORE_PSBANK(tmp);
}

/**
 * Clear rc flash area
 */
void clear_rc_flash(void) banked
{
  u8_t i;

  /* Clear flash pages needed to store remote codes */
  for (i=0;i<(MAX_RC_STORE - RC_STORAGE)/1024;i++) {
    A_(dprintf(APP_BANK, "Erasing bank at: %04x\r\n", RC_STORAGE + i*1024);)
    erase_config_area((xdata u8_t*)(RC_STORAGE + i*1024), APP_BANK);
  }
}

/**
 * check_remote_flash
 * this function checks the remote code storage. if it's not correct (empty) it
 * will initialize it with RC-5 codes
 */
void check_remote_flash(void)
{
  u16_t i;
  u8_t tmp;
  u8_t not_ff = FALSE;

  /* Check all positions for 0xff. If all are 0xff, we need to do a
   * restore of RC-5 codes */
  SAVE_PSBANK(tmp);
  SET_PSBANK(APP_BANK);

  A_(printf("Checking FLASH....\r\n");)
  for (i=RC_STORAGE; i<MAX_RC_STORE ; i++) {
    if (*(code u8_t*)i != 0xff) {
      not_ff = TRUE;
      break;
    }
  }

  /* Now, do we need to restore ? */
  if (!not_ff) {
    copy_rc_to_flash();
  }
  RESTORE_PSBANK(tmp);
}

/*
 * Remote control decoder thread
 *
 * This thread will wait until a full remote control word have been read into
 * the buffer and decode it.
 */
PT_THREAD(handle_remote(struct cremote *Remote) banked __reentrant)
{
  PT_BEGIN(&Remote->pt);

  check_remote_flash();

  Remote->detect_timer = 0;
  Remote->repeat_timer = 0;
  Remote->repeat_status = RPT_NO_REPEAT;

  while (1)
  {
    PT_WAIT_UNTIL(&Remote->pt, SIG_REMOTE_PROCESS == 1);

    SIG_REMOTE_PROCESS = 0;

    /* Get the length for the current package and store it */
    Remote->length = remote_get_packet_length();

    /* Okey, now we are going to try and find the remote code in our stored list */
    {
      char entry = 0;
      const u16_t *cptr;
      u8_t j, i;
      u8_t tmp;
      u8_t found = FALSE;

      /* We need to point out the correct flash bank for getting our constants */
      SAVE_PSBANK(tmp);
      SET_PSBANK(APP_BANK);
      for (j=0 ; j<MAX_KEY ; j++) {
        cptr = (code u16_t*)(RC_STORAGE + j * 128);
        /* Does number of edges match ? */
        C_(printf("Entry %d, Packet Length %d, in flash %d ! ", j, Remote->length , *cptr);)
        if (Remote->length == *cptr)
        {
          cptr++;           /* Skip the length entry */
          found = TRUE;     /* For now we assume it will be ok */
          for (i=1; i<Remote->length ; i++)
          {
            if (time_array[i] < (*cptr-200) ||
                time_array[i] > (*cptr+200))
            {
              C_(printf("Bailing on key %d, entry %d, %04x != %04x, diff=%d\r\n", j, i, time_array[i], *cptr);)
              found = FALSE;
              break;
            }
            cptr++;
          }
        }
        /* Check if a code was found in the table, in that case exit the loop */
        if (found)
          break;
        entry++;
      }
      /* Restore PSBANK */
      B_(printf("Found something, found=%d, entry=%d\r\n", found, entry);)
      if (found) {
        if (last_key != entry) {
          Remote->detect_timer = 0;
          Remote->repeat_timer = 0;
          Remote->repeat_status = RPT_NO_REPEAT;
          last_key = entry;
          /* Check if the key has a long press attribute, this is handled further down */
          if (!(key_attributes[entry] & KEY_LONG_PRESS)) {
            key_code = entry >> 1;
          }
        } else {
          /* Same key pressed, try to detect repeat timeout */
          if (key_attributes[entry] & KEY_REPEAT) {
            if (Remote->repeat_status == RPT_NO_REPEAT) {
              Remote->detect_timer++;
              if (Remote->detect_timer == sys_cfg.remote_repeat_time) {
                Remote->detect_timer = 0;
                Remote->repeat_timer = 0;
                Remote->repeat_status = RPT_REPEAT_ON;
                last_key = entry;
                key_code = entry >> 1;
              }
            } else {
              Remote->repeat_timer++;
              if (Remote->repeat_timer == sys_cfg.remote_repeat_rate) {
                Remote->repeat_timer = 0;
                last_key = entry;
                key_code = entry >> 1;
              }
            }
          } else if (key_attributes[entry] & KEY_LONG_PRESS) {
            if (Remote->repeat_status != RPT_LONG_PRESS_REACHED) {
              /* We use the same timer for long press as for initial repeat time out */
              Remote->detect_timer++;
              if (Remote->detect_timer == sys_cfg.remote_long_press) {
                B_(printf("Long press detected, entry=%d\r\n", entry);)
                Remote->detect_timer = 0;
                //Remote->repeat_status = RPT_LONG_PRESS_REACHED;
                last_key = entry;
                key_code = entry >> 1;
              }
            }
          }
        }
      }
      RESTORE_PSBANK(tmp);
    }
  }
  PT_END(&Remote->pt);
}

/**
 * This function compares the time_array entry to the average array
 * to see if there is a statistical match.
 */
u8_t decoded_key(void)
{
  u8_t found;
  u8_t i;

  found = TRUE;     /* For now we assume it will be ok */
  /* Start match from entry 1 since entry 0 will always be 0x0000 */
  for (i=1; i<remote_get_packet_length() ; i++)
  {
    if (time_array[i] < average[i] - 200 ||
        time_array[i] > average[i] + 200)
    {
      found = FALSE;
      break;
    }
  }
  return found;
}

/**
 * Routine to output data on the LED display.
 */
void out_str(struct remote_trainer *rtm, const char* str) __reentrant
{
  u8_t tmp;
  /* Strings are in paged flash */
  SAVE_PSBANK(tmp);
  SET_PSBANK(APP_BANK);
  /* Copy string to XRAM */
  memcpy(rtm->ledstr, str, 3);
  /* Get back the old flash page */
  RESTORE_PSBANK(tmp);
  /* Output string to led display */
  led_out(rtm->ledstr);
}

/**
 * flash_key
 * This function writes the specified remote key code sequence to the FLASH
 * memory.
 */
void flash_key(u16_t key, u8_t len)
{
  xdata u8_t *ptr = (xdata u8_t*)(RC_STORAGE + key * 128);

  A_(dprintf(APP_BANK, "Writing to flash at address %p\r\n", ptr);)

  /* The first entry in the rc code entry holds the number of codes */
  average[0] = (u16_t)len;
  write_to_flash(ptr, (u8_t *)&average[0], len * 2, APP_BANK);
}

/**
 * This is the remote control trainer process.
 */
static const char *keystr[] = {" 0", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9",
                               "rd", "6r", "4l", "bl", "UP", "dn", "Pr" };
PT_THREAD(handle_rtm(struct remote_trainer *rtm) __reentrant banked)
{
  PT_BEGIN(&rtm->pt);

  /* Clear display while erasing the banks needed */
  out_str(rtm, "  ");
  clear_rc_flash();

  /* First key to learn */
  rtm->key = 0;
  out_str(rtm, keystr[rtm->key]);

  while (1) {
    long avg;

    /* Clear the averaging buffer */
    memset(average, 0,  sizeof(average));

    /* Do averaging */
    for (i=0 ; i<AVERAGE_LENGTH ; i++) {
      /* Wait for a remote keycode to arrive */
      PT_WAIT_UNTIL(&rtm->pt, SIG_REMOTE_PROCESS);
      SIG_REMOTE_PROCESS = 0;

      /* get and store current package length */
      rtm->length = remote_get_packet_length();

      /* First check if this is a valid key code */
      if (rtm->length > MAX_CODE_ENTRIES) {
        out_str(rtm, "E1");
        rtm->timer = alloc_timer();
        set_timer(rtm->timer, 250, NULL);
        PT_WAIT_UNTIL(&rtm->pt, (get_timer(rtm->timer) == 0));
        ENABLE_REMOTE_TRAINER_MODE = 0;
        PT_EXIT(&rtm->pt);
      }

      /* Cumulative averaging */
      for (j=1; j<rtm->length ; j++) {
        if (!i) {
          /* if the average buffer is empty we need to fill up with some samples */
          average[j] = time_array[j];
          C_(dprintf(APP_BANK, "%04x-%04x ", average[j], time_array[j]);)
        } else {
          avg = ((long)average[j] + (long)time_array[j]) / 2;
          average[j] = (u16_t)avg;
          C_(dprintf(APP_BANK, "%04x-%04x ", average[j], time_array[j]);)
        }
      }
      C_(dprintf(APP_BANK, "\r\n");)
    }

    /* Notify that we are done */
    beep(4000, 10);

    /* Flash the current key configuration */
    A_(dprintf(APP_BANK, "Flashing key %d, length %d\r\n", (int)rtm->key, (int)rtm->length);)
    flash_key (rtm->key, rtm->length);

    /* Next key */
    rtm->key++;
    out_str(rtm, (keystr[(rtm->key >> 1)]));
    if (rtm->key & 0x01)
      /* LED on indicates second key press of a button */
      set_led(ON, 0);
    else
      /* LED off indicates first key press of a button */
      set_led(OFF, 0);

    /* Now wait for the operator to release the button */
    do {
      /* Wait for a remote keycode to arrive */
      PT_WAIT_UNTIL(&rtm->pt, SIG_REMOTE_PROCESS);
      SIG_REMOTE_PROCESS = 0;
    } while (decoded_key());

    if (rtm->key == 34) {
      ENABLE_REMOTE_TRAINER_MODE = 0;
      PT_EXIT(&rtm->pt);
    }
  }

  PT_END(&rtm->pt);
}

/* End of file */
