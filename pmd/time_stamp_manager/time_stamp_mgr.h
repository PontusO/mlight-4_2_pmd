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
#ifndef TIME_STAMP_MGR_H_INCLUDED
#define TIME_STAMP_MGR_H_INCLUDED

/*
 * MAX_EE_ENTRIES    - Defines how many entries of the size DATA_ENTRY_SIZE
 *                     that will fit inside the EEPROM.
 * DATA_ENTRY_SIZE   - Defines the size of one entry. This must be larger
 *                     than the data size and divisable by 16.
 * MAGIC_SIZE        - Defines the size in bytes of the magic word
 * DIR_ENTRY_SIZE    - Defines the size of the directory entry
 * FIRST_FREE_ENTRY  - Defines the first location where data can be
 *                     written to.
 */
#define MAX_EE_ENTRIES    4095
#define DATA_ENTRY_SIZE   16
#define MAGIC_SIZE        8
#define DIR_ENTRY         0x0008
#define DIR_ENTRY_SIZE    8
#define FIRST_FREE_ENTRY  (MAGIC_SIZE + DIR_ENTRY_SIZE)

struct tsm {
  struct pt pt;
  u8_t timer;
};

enum ENTRY_TYPES {
  ENTRY_CHANNEL = 0x00,
  ENTRY_LOGIN,
  ENTRY_POWER
};

enum ENTRY_COMMANDS {
  CMD_CHANNEL_CHANGE = 0x01,
  CMD_LOGIN_IN,
  CMD_LOGIN_OUT,
  CMD_POWER_ON,
  CMD_POWER_OFF
};

struct time_data {
  u8_t entry_type;
  u8_t entry_command;
  unsigned long time_stamp;
  u16_t channel;
  u8_t user;
};

/* This struct must not be allowed to grow over 8 bytes */
struct dir_data {
  u16_t no_entries;
  u16_t next_free;
  u16_t last_channel;
};

struct dir_entry {
  struct pt pt;
  struct dir_data dir_data;
};

extern struct tsm tsm;
extern struct tsm tsm_direct;
extern struct dir_entry dir_entry;
extern struct time_data time_data;
extern struct time_data direct_time_data;
extern bit TSM_EVENT_WRITE_DIRECT_TIME_STAMP;
extern bit TSM_EVENT_WRITE_TIME_STAMP;
extern bit TSM_STARTUP_EVENT;

void init_tsm(void) banked;
u8_t tsm_reset_directory(void) banked;
PT_THREAD(handle_tsm(struct tsm *tsm) banked);
PT_THREAD(handle_tsm_direct(struct tsm *tsm) banked);
PT_THREAD(get_dir_entry(struct dir_entry *de) banked);

#endif // TIME_STAMP_MGR_H_INCLUDED
