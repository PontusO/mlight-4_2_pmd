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

#include "flash.h"
#include "system.h"
#include "string.h"
#include "iet_debug.h"

/* This defines how many bytes there are in a flash page */
#define FLASH_PAGE_SIZE     1024
#define ENTIRE_FLASH        1
#define FLASH_WO_VER        0

/* this is the default config */
const struct sys_config default_cfg = {
  {{0x00, 0x3F, 0xB2, 0xC9, 0x0A, 0x00}}, /* The MAC address of the node */
  {0x30, 0x30, 0x30, 0x30, 0, 0, 0, 0},   /* String with "0000" */
  1,                                      /* Default is the first TV in the house */
  {192, 168, 0, 11},                      /* The IP address of the node */
  {255, 255, 255, 0},                     /* The network mask of the node */
  {192, 168, 0, 1},                       /* The default gateway of the node */
  1,                                      /* The Timer server is enabled per default */
  {192, 168, 0, 45},                      /* Time protocol server */
  37,                                     /* Time protocol server port */
  48,                                     /* Defult update interval = 48 hours */
  2,                                      /* Default time zone */
  10,                                     /* Default repeat timeout */
  5,                                      /* Default repeat rate */
  150,                                    /* default channel select time out */
  450,                                    /* Default we start requesting the user after 4.5 seconds */
  30,                                     /* And we will allow a 30 second window for the user to log in */
  1,                                      /* user stays logged in for this per default */
  50,                                     /* Default channel selection discriminator time */
  { 0, 1, 2, 5 },                         /* Default color key mapping */
  1,                                      /* Upgrades are enabled */
  {224, 10, 1, 0 },                       /* Our default multicast address */
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,         /* Default channel map */
    10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,
    30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,
    50,51,52,53,54,55,56,57,58,59,
    60,61,62,63,64,65,66,67,68,69,
    70,71,72,73,74,75,76,77,78,79,
    80,81,82,83,84,85,86,87,88,89,
    90,91,92,93,94,95,96,97,98,99
  },
  10,                                     /* Default time out for long presses */
  { "admin", 0, 0, 0, 0 },                /* Default user name and password */
  { "pass", 0, 0, 0, 0, 0 }
};

/* This is the config in the RAM */
struct sys_config sys_cfg;

/* Prototypes */
static int flash_chksum(u8_t command);
static void write_new_verification_word(int ver);
void load_network_params(void);

//-----------------------------------------------------------------------------
// Non-interrupt delay approx 1mS to 255mS
//-----------------------------------------------------------------------------
void wait_ms(u8_t count) banked
{
  u16_t i;

  for ( ; count > 0; count--) {
    for ( i = 10000; i > 0; i--);
  }
}

/* config block flash writing mode */
void flash_write_config(void)
{
  ENTER_CRITICAL_SECTION;

  SFRPAGE = CONFIG_PAGE;
  CCH0CN =  CCH0CN & 0x01;  /* set CHBLKW(CCH0CN.0) bit*/

  SFRPAGE = LEGACY_PAGE;

  EXIT_CRITICAL_SECTION;
}

static void enter_flash_write(void)
{
  ENTER_CRITICAL_SECTION;

  SFRPAGE = LEGACY_PAGE;
  FLSCL |= 0x01;          /* set FLWE(FLSCL.0) bit*/
  PSCTL |= 0x01;          /* set PSWE(PSCTL.0) bit*/
}

static void exit_flash_write(void)
{
  SFRPAGE = LEGACY_PAGE;
  PSCTL &= ~0x01;         /* clear PSWE(PSCTL.0) bit*/
  FLSCL &= ~0x01;         /* clear FLWE(FLSCL.0) bit*/

  EXIT_CRITICAL_SECTION;
}

void erase_config_area(xdata u8_t * data erase_ptr, u8_t bank)
{
  u8_t reg_bak;

  ENTER_CRITICAL_SECTION;

  SFRPAGE = LEGACY_PAGE;
  reg_bak = PSBANK;
  PSBANK = (PSBANK & 0xcf) | (bank << 4);

  FLSCL |= 0x01;          /* set FLWE(FLSCL.0) bit*/
  PSCTL |= 0x03;          /* set PSWE(PSCTL.0)  and PSEE(PSCTL.1) bit*/

  *erase_ptr = 0x00;

  PSCTL &= ~0x03;         /* clear PSWE(PSCTL.0)  and PSEE(PSCTL.1) bit*/
  FLSCL &= ~0x01;         /* clear FLWE(FLSCL.0) bit*/

  /* Allow for the device to erase the FLASH page */
  wait_ms(12);

  PSBANK = reg_bak;
  EXIT_CRITICAL_SECTION;
}

/*
 * write_to_flash
 *
 * Function for writing a specified number of bytes to the flash memory
 * The bank must be set up correctly by caller before calling this
 * function.
 */
void write_to_flash(xdata u8_t * data flash_ptr, u8_t *src_ptr,
                    u16_t len, u8_t bank)
{
  u16_t i;
  u8_t count;
  u8_t reg_bak;

  SFRPAGE = LEGACY_PAGE;
  reg_bak = PSBANK;
  PSBANK = (PSBANK & 0xcf) | (bank << 4);

  enter_flash_write();
  /* 4-byte block alignment */
  for (i=0; i<len + 4 - (len & 0x0003) ; i++)
  {
    if (!(i & 0x0003))
    {
      for (count=100; count>0; count--)
        {}  /* delay 60us, wait for the writing operation */
    }
    *flash_ptr++=*src_ptr++;
  }
  for (count=100; count>0; count--)
    {}  /* last 4-byte writing delay */
  exit_flash_write();

  PSBANK = reg_bak;
}

/* save config to Flash */
void write_config_to_flash(void)
{
  int chksum;

  /* Erase the configuration flash block */
  erase_config_area(LAST_PAGE_ADDRESS, 0x01);
  write_to_flash(LAST_PAGE_ADDRESS, (xdata u8_t *)&sys_cfg, CONFIG_MEM_SIZE, CONST_BANK);

  // Get new check sum of entire block minus last word
  chksum = flash_chksum(FLASH_WO_VER);
  B_(printf_small("New FLASH checksum %d, verfication word %d\r\n", (int)chksum, (int)(-1 - chksum));)
  write_new_verification_word(-1 - chksum);
}

static void write_new_verification_word(int ver)
{
  u8_t count;
  __idata u8_t reg_bak;
  __xdata u8_t * __idata dest = (__xdata u8_t *)LAST_PAGE_ADDRESS + 0x3FE;

  reg_bak = PSBANK;
  PSBANK = ((PSBANK | 0x10) & 0xDF); /* select COBANK = 01*/

  SFRPAGE = CONFIG_PAGE;
  CCH0CN =  CCH0CN & 0x01;  /* clear CHBLKW(CCH0CN.0) bit*/
  SFRPAGE = LEGACY_PAGE;

  enter_flash_write();

  *dest++ = ver & 0xff;
  for (count=100; count>0; count--);
  *dest = ver >> 8;
  for (count=100; count>0; count--);

  exit_flash_write();

  PSBANK = reg_bak;
}

/**
 * This method calculates the checksum of the configuration block.
 *
 * @return the sum of the entire block minus the verification word.
 */
static int flash_chksum(u8_t command)
{
  int i;
  __code int *dest = (__code int *)LAST_PAGE_ADDRESS;
  int chksum = 0;
  int loop;

  switch (command)
  {
    case FLASH_WO_VER:
      loop = (FLASH_PAGE_SIZE/2)-1;
      break;

    case ENTIRE_FLASH:
      loop = FLASH_PAGE_SIZE/2;
      break;

    default:
      loop = FLASH_PAGE_SIZE/2;
      break;
  }

  B_(printf("Iterating %d\r\n", (int)loop);)
  for (i=0 ; i<loop ; i++)
  {
    chksum += *dest++;
  }

  return chksum;
}

/**
 * This method will check that the checksum of the configuration flash block is
 * valid. If it is not valis it will replace the content with a valid default
 * content to ensure proper operation. It will return an error code to report that
 * it had to do the update.
 *
 * Ths checksum is calculated by adding every byte in the config block together.
 * This should return -1 as a result if all is OK. Anything else is incorrect.
 *
 * This method uses some sneaky memory allocations since it is called before
 *
 * @return ERR_CONFIG_FLASH_NOT_VALID If the config  block was invalid and needed
 *         an update.
 **/

u8_t validate_config_flash(void)
{
  int chksum = 0;
  __code int *memptr = (__code int *)LAST_PAGE_ADDRESS;

  chksum = flash_chksum(ENTIRE_FLASH);
  B_(printf_small("FLASH checksum %d\r\n", (int)chksum);)

  /* Check if the content is valid */
  if (chksum != -1) {
    B_(printf_small("Initializing FLASH\r\n");)
    // Go and get the default values from flash
    load_default_config();
    // Write it to flash
    write_config_to_flash();
  } else {
    B_(printf_small("Loading existing FLASH configuration\r\n");)
    load_sys_config();
  }
  load_network_params();

  return 0;
}

/* This is the config saved in the FLASH.
We put it in the last page of the 64K flash. (BANK0 + COBANK1).
According to the SDCC manual section 3.5, The compiler won't generate the code
for a variable or const declared using absolute addressing. */
static const __at(LAST_PAGE_ADDRESS) u8_t saved_cfg[1024]={0x0};

/*
 * This function loads network parameters into the tcp/ip stack.
 * Should probably be somewhere else but will do for now.
 */
void load_network_params(void)
{
  uip_ipaddr_t addr;

  uip_setethaddr(sys_cfg.mac_addr);
  /* A special setting that allows multiple units to be on the same network */
  uip_ethaddr.addr[5] = sys_cfg.tv_number;

  uip_ipaddr(&addr, sys_cfg.ip_addr[0],
             sys_cfg.ip_addr[1],
             sys_cfg.ip_addr[2],
             sys_cfg.ip_addr[3]);
  uip_sethostaddr(&addr);

  uip_ipaddr(&addr, sys_cfg.netmask[0],
             sys_cfg.netmask[1],
             sys_cfg.netmask[2],
             sys_cfg.netmask[3]);
  uip_setnetmask(&addr);

  uip_ipaddr(&addr, sys_cfg.gw_addr[0],
             sys_cfg.gw_addr[1],
             sys_cfg.gw_addr[2],
             sys_cfg.gw_addr[3]);
  uip_setdraddr(&addr);
}

/* load the system config from the flash when the system is powered on. */
void load_sys_config(void)
{
  /* copy config from flash to xram*/
  memcpy(&sys_cfg, &saved_cfg, CONFIG_MEM_SIZE);
}

/* this function simply loads the default configuration values
 * into the global configuration structure
 */
void load_default_config(void)
{
  memcpy(&sys_cfg, &default_cfg, CONFIG_MEM_SIZE);
}

// EOF