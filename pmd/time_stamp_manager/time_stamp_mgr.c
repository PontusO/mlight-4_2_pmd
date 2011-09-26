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
#pragma codeseg APP_BANK

#include "system.h"
#include "swtimers.h"
#include "flash.h"
#include "time_stamp_mgr.h"
#include "i2c.h"
#include "sound.h"
#include "iet_debug.h"
#include "theapp.h"
#include "led.h"

#include <stdio.h>
#include <string.h>

/*
 * Here for now, will be moved later
 */
struct deadbeef {
  struct pt pt;
};
struct deadbeef deadbeef;
static const char str_deadbeef[] = "deadbeef";

/* Application Data */
struct tsm tsm;
struct tsm tsm_direct;
struct time_data time_data;
struct time_data direct_time_data;
struct dir_entry dir_entry;

static u8_t ee_buf[16];
bit TSM_EVENT_WRITE_TIME_STAMP;
bit TSM_STARTUP_EVENT;
bit TSM_EVENT_WRITE_DIRECT_TIME_STAMP;

/**
 * init_tsm
 *
 * Initializes the tsm application.
 */
void init_tsm(void) banked
{
  TSM_EVENT_WRITE_TIME_STAMP = 0;
  TSM_STARTUP_EVENT = 0;
  TSM_EVENT_WRITE_DIRECT_TIME_STAMP = 0;

  /* Clear time data buffers */
  memset(&time_data, 0 , sizeof (struct time_data));
  memset(&direct_time_data, 0 , sizeof (struct time_data));

  PT_INIT(&tsm.pt);
  PT_INIT(&tsm_direct.pt);
}

/**
 * tsm_reset_directory
 *
 * This function resets the directory structure.
 */
u8_t tsm_reset_directory(void) banked
{
  struct dir_data dd;

  dd.no_entries = 0;
  dd.next_free = FIRST_FREE_ENTRY;
  dd.last_channel = 0x01;

  /* Setup I2C data structure to initialize EEPROM */
  i2c.address = DIR_ENTRY;
  i2c.device = EEPROM1;
  i2c.buffer = (u8_t*)&dd;
  i2c.len = sizeof(struct dir_data);

  /* Write the data */
  return(nos_i2c_write(&i2c));
}

/**
 * get_dir_entry
 *
 * This thread fetches the directory entry from the EEPROM and stores
 * it in a local array. the direntry is located from 0x0008 - 0x000F
 */
PT_THREAD(get_dir_entry(struct dir_entry *de) banked)
{
  PT_BEGIN(&de->pt);
#ifndef USE_UART_INSTEAD_OF_SMB
  /* First make sure the I2C interface is free */
  PT_WAIT_UNTIL(&de->pt, (i2c.busy == FALSE));

  /* Setup I2C data structure to read the direntry */
  i2c.address = 0x0008;
  i2c.device = EEPROM1;
  i2c.buffer = (u8_t*)&de->dir_data;
  i2c.len = sizeof(struct dir_data);

  /* Do the I2C transfer */
  PT_SPAWN(&de->pt, &i2c.pt, SM_Receive(&i2c));
#else
  A_(printf("Reading from the directory entry\r\n");)
  de->dir_data.no_entries = 25;
  de->dir_data.next_free = 416;
  B_(printf("Returning dummy values\r\n");)
  B_(printf("  no_entries = %u\r\n", de->dir_data.no_entries);)
  B_(printf("  next_free  = %u\r\n", de->dir_data.next_free);)
#endif
  PT_END(&de->pt);
}

/**
 * save_dir_entry
 *
 * This thread saves the specified directory entry to EEPROM.
 * The direntry is located from 0x0008 - 0x000F
 */
PT_THREAD(save_dir_entry(struct dir_entry *de))
{
  PT_BEGIN(&de->pt);
#ifndef USE_UART_INSTEAD_OF_SMB
  /* First make sure the I2C interface is free */
  PT_WAIT_UNTIL(&de->pt, (i2c.busy == FALSE));

  /* Setup I2C data structure to write the direntry */
  i2c.address = 0x0008;
  i2c.device = EEPROM1;
  i2c.buffer = (u8_t*)&de->dir_data;
  i2c.len = sizeof(struct dir_data);

  /* Do the I2C transfer */
  PT_SPAWN(&de->pt, &i2c.pt, SM_Send(&i2c));
#else
  A_(printf("Writing to the directory entry\r\n");)
  B_(printf("  no_entries = %u\r\n", de->dir_data.no_entries);)
  B_(printf("  next_free  = %u\r\n", de->dir_data.next_free);)
#endif
  PT_END(&de->pt);
}

/**
 * do_deadbeef
 *
 * Reads the first 8 bytes of the EEPROM and compares it to the
 * string "deadbeef". If it matches nothing happens, if not we write
 * the string and initialize the directory entry.
 */
static PT_THREAD(do_deadbeef(struct deadbeef *db))
{
  PT_BEGIN(&db->pt);
#ifndef USE_UART_INSTEAD_OF_SMB
  /* First make sure the I2C interface is free */
  PT_WAIT_UNTIL(&db->pt, (i2c.busy == FALSE));

  /* Setup I2C data structure to read the magic entry */
  i2c.address = 0x0000;
  i2c.device = EEPROM1;
  i2c.buffer = (u8_t*)ee_buf;
  i2c.len = 8;

  /* Do the I2C transfer */
  PT_SPAWN(&db->pt, &i2c.pt, SM_Receive(&i2c));

  if (strncmp(ee_buf, str_deadbeef, 8) != 0) {
    struct dir_data *dd = (struct dir_data*)(ee_buf + 8);
    /* Did not find deadbeef in the EEPROM */
    memcpy(ee_buf, str_deadbeef, 8);
    dd->no_entries = 0;
    dd->next_free = FIRST_FREE_ENTRY;
    dd->last_channel = 1;

    /* Setup I2C data structure to initialize EEPROM */
    i2c.address = 0x0000;
    i2c.device = EEPROM1;
    i2c.buffer = (u8_t*)ee_buf;
    i2c.len = 16;

    /* Write the data */
    PT_SPAWN(&db->pt, &i2c.pt, SM_Send(&i2c));
  }
#else
  A_(printf("Validating MAGIC entry\r\n");)
#endif
  PT_END(&db->pt);
}

/**
 * handle_tsm
 *
 * This is the protothread of the time stamp manager.
 * Here we will serve a number of events ending up in writing data to
 * or reading from the EEPROM.
 */
PT_THREAD(handle_tsm(struct tsm *tsm) banked)
{
  PT_BEGIN(&tsm->pt);

  while (1) {
    /* Wait for an event to arrive */
    PT_WAIT_UNTIL(&tsm->pt, (TSM_EVENT_WRITE_TIME_STAMP ||
                             TSM_STARTUP_EVENT));
    if (TSM_EVENT_WRITE_TIME_STAMP) {
      /* Clear event */
      TSM_EVENT_WRITE_TIME_STAMP = 0;
      A_(printf("TSM Event\r\n");)
      /* A write to EEPROM event occured */
      /* First wait for the discriminator time to time out */
      tsm->timer = alloc_timer();
      set_timer(tsm->timer, sys_cfg.discr_time * 10, NULL);
      /* Wait here until timeout, or new event
       * If a new event arrives, we can throw away the old data
       * and start the discriminator timout again.
       */
      PT_WAIT_UNTIL(&tsm->pt, get_timer(tsm->timer) == 0 ||
                    TSM_EVENT_WRITE_TIME_STAMP);
      free_timer(tsm->timer);
      if (TSM_EVENT_WRITE_TIME_STAMP)
        PT_RESTART(&tsm->pt);
      /* Go and get the directory entry */
      PT_SPAWN(&tsm->pt, &dir_entry.pt, get_dir_entry(&dir_entry));

      /* Check that there is enough room to fit the entry */
      if (dir_entry.dir_data.no_entries < MAX_EE_ENTRIES) {
#ifndef USE_UART_INSTEAD_OF_SMB
        /* Make sure the I2C interface is free */
        PT_WAIT_UNTIL(&tsm->pt, i2c.busy == FALSE);

        /* Setup to write data */
        i2c.address = dir_entry.dir_data.next_free;
        i2c.device = EEPROM1;
        i2c.buffer = (u8_t*)time_data;
        i2c.len = sizeof(struct time_data);
        /* Write the time data */
        PT_SPAWN(&tsm->pt, &i2c.pt, SM_Send(&i2c));

        dir_entry.dir_data.no_entries++;
        dir_entry.dir_data.next_free += DATA_ENTRY_SIZE;
        /* We alse save the selected channel */
        dir_entry.dir_data.last_channel = time_data.channel;

        /* Go and save new directory entry */
        PT_SPAWN(&tsm->pt, &dir_entry.pt, save_dir_entry(&dir_entry));
#else
        A_(printf("Writing new channel entry to EEPROM\r\n");)
#endif
      } else {
        /* Perhaps we should do something here when the EEPROM is full */
        /*
         * Beep for now
         */
        beep(1000, 100);
      }
    } else if (TSM_STARTUP_EVENT) {
      /* We have received a startup event, do some initial stuff */

      /* First, make sure EEPROM file system is OK */
      PT_SPAWN(&tsm->pt, &deadbeef.pt, do_deadbeef(&deadbeef));

      /* Now, read the direntry to get the last selected channel */
      /* Go and get the directory entry */
      PT_SPAWN(&tsm->pt, &dir_entry.pt, get_dir_entry(&dir_entry));

      /* Suggest this new channel to the main application */
      suggest_channel(dir_entry.dir_data.last_channel);
      /* Done here */
      TSM_STARTUP_EVENT = 0;
    }
  }

  PT_END(&tsm->pt);
}

/**
 * handle_tsm_quick
 *
 * This is the protothread of the second part of the time stamp manager.
 * The thread will take care of writing entries that can to be written directly
 * tp the EEPROM without any delay.
 */
PT_THREAD(handle_tsm_direct(struct tsm *tsm) banked)
{
  PT_BEGIN(&tsm->pt);

  while (1) {
    /* Wait for an event to arrive */
    PT_WAIT_UNTIL(&tsm->pt, (TSM_EVENT_WRITE_DIRECT_TIME_STAMP));
    TSM_EVENT_WRITE_DIRECT_TIME_STAMP = 0;

    /* Go and get the directory entry */
    PT_SPAWN(&tsm->pt, &dir_entry.pt, get_dir_entry(&dir_entry));

    /* Check that there is enough room to fit the entry */
    if (dir_entry.dir_data.no_entries < MAX_EE_ENTRIES) {

#ifndef USE_UART_INSTEAD_OF_SMB
      /* Make sure the I2C interface is free */
      PT_WAIT_UNTIL(&tsm->pt, i2c.busy == FALSE);

      /* Setup to write data */
      i2c.address = dir_entry.dir_data.next_free;
      i2c.device = EEPROM1;
      i2c.buffer = (u8_t*)direct_time_data;
      i2c.len = sizeof(struct time_data);
      /* Write the time data */
      PT_SPAWN(&tsm->pt, &i2c.pt, SM_Send(&i2c));

      dir_entry.dir_data.no_entries++;
      dir_entry.dir_data.next_free += DATA_ENTRY_SIZE;
      /* We alse save the selected channel */
      dir_entry.dir_data.last_channel = direct_time_data.channel;

      /* Go and save new directory entry */
      PT_SPAWN(&tsm->pt, &dir_entry.pt, save_dir_entry(&dir_entry));
#else
      A_(printf("Writing new direct entry to EEPROM\r\n");)
#endif
    } else {
      /* Perhaps we should do something here when the EEPROM is full */
      /*
       * Beep for now
       */
      beep(1000, 100);
    }
  }

  PT_END(&tsm->pt);
}

