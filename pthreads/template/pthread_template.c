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

/* PTHREAD template code
 *
 * This file holds a template that can be used to create pthread
 * threads. Simple replace all occurances of pthread_template in this
 * file and in the header file and you have a new pthread.
 *
 * Also remember to schedule your pthread somewhere
 */
//#pragma codeseg APP_BANK
//#define PRINT_A     // Enable A prints

#include "system.h"
#include "iet_debug.h"
#include "pthread_template.h"

/*
 * Initialize the pthread_template pthread
 */
void init_pthread_template(pthread_template_t *pthread_template) __reentrant __banked
{
  PT_INIT(&pthread_template->pt);
}

PT_THREAD(handle_pthread_template(pthread_template_t *pthread_template) __reentrant __banked)
{
  PT_BEGIN(&pthread_template->pt);

  printf ("Starting pthread_template pthread!\n");

  while (1)
  {
  }

  PT_END(&pthread_template->pt);
}

/* EOF */
