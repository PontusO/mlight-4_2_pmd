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
#include <string.h>

#include "system.h"
#include "dac.h"

u8_t dac_table[2];

/*
 * Initialize the dac driver
 */
void init_dacs (void)
{
  memset (&dac_table, 0, sizeof dac_table);
}

/*
 * Allocate a dac.
 *
 * When running this function the dac allocation table will be
 * checked to make sure that the requested dac is available.
 * If not, an error code will be returned to indicate the fault.
 * The dac HW will then be initialized.
 */
char allocate_dac (u8_t dac)
{
  /* Sanity check of input parameter */
  if (dac >= DAC_NUM_DACS)
    return DAC_ERR_NO_SUCH_DEVICE;

  if (dac_table[dac] == DAC_ALLOCATED)
    return DAC_ERR_ALREADY_IN_USE;
  dac_table[dac] = DAC_ALLOCATED;

  switch (dac)
  {
    case 0:
      SFRPAGE = DAC0_PAGE;
      DAC0CN = DAC_ENABLE | DAC_UPDATE_DAC_H | DAC_MODE_0;
      break;

    case 1:
      SFRPAGE = DAC1_PAGE;
      DAC1CN = DAC_ENABLE | DAC_UPDATE_DAC_H | DAC_MODE_0;
      break;

    default:
      return DAC_ERR_NO_SUCH_DEVICE;
  }
  return DAC_ERR_OK;
}

/*
 * Free dac
 *
 * The dacs are general ouput types that could possibly be used for
 * a multitude of different applications. Therefore every sensor application must
 * free the dac when it is not being used. So that other applications
 * can make use of the dac.
 */
char free_dac (u8_t dac)
{
  /* Sanity check of input parameter */
  if (dac >= DAC_NUM_DACS)
    return DAC_ERR_NO_SUCH_DEVICE;

  dac_table[dac] = DAC_FREE;

  switch (dac)
  {
    case 0:
      SFRPAGE = DAC0_PAGE;
      DAC0CN = DACXCN_DEFAULT_VALUE;
      break;

    case 1:
      SFRPAGE = DAC1_PAGE;
      DAC1CN = DACXCN_DEFAULT_VALUE;
      break;

    default:
      return DAC_ERR_NO_SUCH_DEVICE;
  }
  return DAC_ERR_OK;
}

/*
 * Write dac
 *
 * This function will write the supplied parameter to the specified dac.
 * -1 will be returned if there was an error with the write operation.
 */
char write_dac (u8_t dac, u16_t value)
{
  switch (dac)
  {
    case 0:
      SFRPAGE = DAC0_PAGE;
      DAC0 = value;
      break;

    case 1:
      SFRPAGE = DAC1_PAGE;
      DAC1 = value;
      break;

    default:
      return DAC_ERR_NO_SUCH_DEVICE;
  }
  return DAC_ERR_OK;
}

/* EOF */
