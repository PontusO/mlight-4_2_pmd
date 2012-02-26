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

#include <string.h>

#include "system.h"
#include "comparator.h"

u8_t comparator_table[2];

/*
 * Initialize the comparator driver
 */
void init_comparators (void)
{
  memset (&comparator_table, 0, sizeof comparator_table);
}

/*
 * Allocate a comparator.
 *
 * When running this function the comparator allocation table will be
 * checked to make sure that the requested comparator is available.
 * If not, an error code will be returned to indicate the fault.
 * The comparator HW will then be initialized.
 */
char allocate_comparator (u8_t comp)
{
  /* Sanity check of input parameter */
  if (comp >= COMP_NUM_COMPARATORS)
    return COMP_ERR_NO_SUCH_DEVICE;

  if (comparator_table[comp] == COMP_ALLOCATED)
    return COMP_ERR_ALREADY_IN_USE;
  comparator_table[comp] = COMP_ALLOCATED;

  switch (comp)
  {
    case 0:
      SFRPAGE = CPT0_PAGE;
      CPT0CN = COMP_ENABLE | COMP_NEGHYS_15MV | COMP_POSHYS_15MV;
      CPT0MD = 0;
      break;

    case 1:
      SFRPAGE = CPT1_PAGE;
      CPT1CN = COMP_ENABLE | COMP_NEGHYS_15MV | COMP_POSHYS_15MV;
      CPT1MD = 0;
      break;

    default:
      return COMP_ERR_NO_SUCH_DEVICE;
  }
  return COMP_ERR_OK;
}

/*
 * Free comparator
 *
 * The comparators are general input types that could possibly be used for
 * a multitude of different sensors. Therefore every sensor application must
 * free the comparator when it is not being used. So that other applications
 * can make use of the comparator.
 */
char free_comparator (u8_t comp)
{
  /* Sanity check of input parameter */
  if (comp >= COMP_NUM_COMPARATORS)
    return COMP_ERR_NO_SUCH_DEVICE;

  comparator_table[comp] = COMP_FREE;

  switch (comp)
  {
    case 0:
      SFRPAGE = CPT0_PAGE;
      CPT0CN = CPTXCN_DEFAULT_VALUE;
      CPT0MD = CPTXMD_DEFAULT_VALUE;
      break;

    case 1:
      SFRPAGE = CPT1_PAGE;
      CPT1CN = CPTXCN_DEFAULT_VALUE;
      CPT1MD = CPTXMD_DEFAULT_VALUE;
      break;

    default:
      return COMP_ERR_NO_SUCH_DEVICE;
  }
  return COMP_ERR_OK;
}

/*
 * Get the current state of the comparator output bit. If an error
 * occured -1 i returned.
 */
char get_comparator_state (u8_t comp)
{
  switch (comp)
  {
    case 0:
      SFRPAGE = CPT0_PAGE;
      return CPT0CN & COMP_OUT ? 1 : 0;

    case 1:
      SFRPAGE = CPT1_PAGE;
      return CPT1CN & COMP_OUT ? 1 : 0;

    default:
      return -1;
  }
}

/* EOF */
