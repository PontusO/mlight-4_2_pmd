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

#include "system.h"       // System description
#include "main.h"         // Locals
#include "iet_debug.h"    // Debug macros
#include "led.h"
#include "swtimers.h"     // Software timer system
#include "flash.h"
#include "i2c.h"
#include "rtc.h"
#include "httpd-cgi.h"
#include "pmd.h"
#include "sound.h"

static u8_t num;
static char lled[3];                  // Must include one extra position for NULL character

extern static u16_t half_Sec;
extern static u16_t ten_Secs;
extern char digit[2];
extern static near u8_t tmcnt;

extern bit TX_EventPending;	          // the DM9000 hardware receive event
extern bit ARP_EventPending;          // trigger the arp timer event
extern bit digit_select;
extern bit callback_kicker;

#define UPDATE_INTERVAL   25

void cleanup( void );
extern void config();
void Timer0_Init (void);

void update_led(unsigned char n)
{
  sprintf(lled, "%02d", n);
  led_out(lled);
}

/*
 * This function is only for start up sequence diagnostics.
 * It should be removed before relase.
 */
void diag_led(unsigned char n)
{
#ifdef DEBUG_STARTUP
  u16_t i,j;
#endif

  update_led(n);
  P1 = digit[1];
#ifdef DEBUG_STARTUP
  for (i=0;i < 5;i++) {
    for (j=0;j<65530;j++) {
      P1 = digit[1];
    }
  }
#endif
}

// ---------------------------------------------------------------------------
//	pmd()
//
//	This is the PMD application
// ---------------------------------------------------------------------------
void pmd(void) banked
{
  unsigned int i;
  unsigned char update = UPDATE_INTERVAL;

  num = 0;
  callback_kicker = 0;

  config();                 // Configure the MCU
  P0 |= DIGIT_FLIPPER;      // Display startup sequence on digit 0
  diag_led(num++);          // Display diagnostics digit "0"

  half_Sec = UIP_TX_TIMER;
  ten_Secs = UIP_ARP_TIMER;

  Timer0_Init();            // 10 mSec interrupt rate

  diag_led(num++);          // Display diagnostics digit "1"
#ifdef USE_UART_INSTEAD_OF_SMB
  init_uart();              // Set the Uart up for operation
#else
  init_i2c();
#endif

  diag_led(num++);          // Display diagnostics digit "2"
  diag_led(num++);          // Display diagnostics digit "3"
#ifdef HAVE_SOUND
  init_sound();
#endif

  diag_led(num++);          // Update diagnistics number "4"
  uip_init();               // Initialise the uIP TCP/IP stack.

  diag_led(num++);          // Display diagnostics digit "5"
#ifdef HAVE_FLASH
  validate_config_flash();  // before we do anything else do this.
#endif

  diag_led(num++);          // Display diagnostics digit "6"
  init_swtimers();          // Initialize all software timers

  diag_led(num++);          // Display diagnostics digit "7"

  diag_led(num++);          // Display diagnostics digit "8"
  init_led();               // Initialize the led handler

  diag_led(num++);          // Display diagnostics digit "9"
  httpd_init();             // Initialise the webserver app.

  diag_led(num++);          // Update diagnistics number "0"
  if (InitDM9000())         // Initialise the device driver.
    cleanup();              // exit if init failed

  diag_led(num++);          // Display diagnostics digit "1"
  uip_arp_init();           // Initialise the ARP cache.

  TX_EventPending = FALSE;	// False to poll the DM9000 receive hardware
  ARP_EventPending = FALSE;	// clear the arp timer event flag

  EA = TRUE;                // Enable interrupts

  A_(printf("\r\n");)
  A_(printf("Invector Embedded Technologies Debug system output v. 1.001\r\n");)
  A_(printf("System: iFutura PMD IET9121, 20MHz system clock, DM9000E Ethernet Controller\r\n");)
  A_(printf("Current Host Settings:\r\n");)
  A_(printf("  IP Address: %d.%d.%d.%d\r\n",
    (u16_t)(htons(uip_hostaddr[0]) >> 8),
    (u16_t)(htons(uip_hostaddr[0]) & 0xff),
    (u16_t)(htons(uip_hostaddr[1]) >> 8),
    (u16_t)(htons(uip_hostaddr[1]) & 0xff));)
  A_(printf("  Default Router: %d.%d.%d.%d\r\n",
    (u16_t)(htons(uip_draddr[0]) >> 8),
    (u16_t)(htons(uip_draddr[0]) & 0xff),
    (u16_t)(htons(uip_draddr[1]) >> 8),
    (u16_t)(htons(uip_draddr[1]) & 0xff));)
  A_(printf("  Netmask: %d.%d.%d.%d\r\n",
    (u16_t)(htons(uip_netmask[0]) >> 8),
    (u16_t)(htons(uip_netmask[0]) & 0xff),
    (u16_t)(htons(uip_netmask[1]) >> 8),
    (u16_t)(htons(uip_netmask[1]) & 0xff));)
  A_(printf("  Network address: %02x-%02x-%02x-%02x-%02x-%02x\r\n",
    (u16_t)uip_ethaddr.addr[0],(u16_t)uip_ethaddr.addr[1],(u16_t)uip_ethaddr.addr[2],
    (u16_t)uip_ethaddr.addr[3],(u16_t)uip_ethaddr.addr[4],(u16_t)uip_ethaddr.addr[5]);)

  diag_led(num++);          // Update diagnistics number "2"
  init_rtc();               // Initialize the RTC

  diag_led(num++);          // Update diagnistics number "3"

  /* Turn off dp */
  P1 = 0x80;
  beep(4000, 20);

  while(1)
  {
    // Loops here until either a packet has been received or
    // TX_EventPending (half a sec) to scan for anything to send or
    // ARP_EventPending (10 secs) to send an ARP packet
    uip_len = DM9000_receive();

    // If uip_len is 0, no packet has been received
    if (uip_len == 0)
    {
      // Test for anything to be sent to 'net
      if (TX_EventPending)
      {
        TX_EventPending = FALSE;  // First clear flag if set

        // UIP_CONNS - nominally 10 - is the maximum simultaneous
        // connections allowed. Scan through all 10
        C_(printf_small("Time for connection periodic management\x0a\x0d");)
        for (i = 0; i < UIP_CONNS; i++)
        {
          uip_periodic(i);
          // If uip_periodic(i) finds that data that needs to be
          // transmitted to the network, the global variable uip_len
          // will be set to a value > 0.
          if (uip_len > 0)
          {
            /* The uip_arp_out() function should be called when an IP packet should be sent out on the
               Ethernet. This function creates an Ethernet header before the IP header in the uip_buf buffer.
               The Ethernet header will have the correct Ethernet MAC destination address filled in if an
               ARP table entry for the destination IP address (or the IP address of the default router)
               is present. If no such table entry is found, the IP packet is overwritten with an ARP request
               and we rely on TCP to retransmit the packet that was overwritten. In any case, the
               uip_len variable holds the length of the Ethernet frame that should be transmitted.
            */
            uip_arp_out();
            tcpip_output();
            tcpip_output();
          }
        }

#if UIP_UDP
        C_(printf_small("Time for udp periodic management\x0a\x0d");)
        for (i = 0; i < UIP_UDP_CONNS; i++)
        {
          uip_udp_periodic(i);
          // If the above function invocation resulted in data that
          // should be sent out on the network, the global variable
          // uip_len is set to a value > 0.
          if (uip_len > 0)
          {
            uip_arp_out();
            tcpip_output();
          }
        }
#endif /* UIP_UDP */
      }
      // Call the ARP timer function every 10 seconds. Flush dead entries
      if (ARP_EventPending)
      {
        B_(printf_small("ARP house keeping.\x0a\x0d");)
        ARP_EventPending = FALSE;
        uip_arp_timer();
      }
    }
    // Packet Received (uip_len != 0) Process incoming
    else
    {
      B_(printf_small("Received incomming data package.\x0a\x0d");)
      if (BUF->type == htons(UIP_ETHTYPE_IP))
      {
        B_(printf_small("IP Package received.\x0a\x0d");)
        // Received an IP packet
        uip_arp_ipin();
        uip_input();
        // If the above function invocation resulted in data that
        // should be sent out on the network, the global variable
        // uip_len is set to a value > 0.
        if (uip_len > 0)
        {
          uip_arp_out();
          tcpip_output();
          tcpip_output();
          B_(printf_small("IP Package transmitted.\x0a\x0d");)
        }
      }
      else
      {
        if (BUF->type == htons(UIP_ETHTYPE_ARP))
        {
          B_(printf_small("ARP package received.\x0a\x0d");)
          // Received an ARP packet
          uip_arp_arpin();
          // If the above function invocation resulted in data that
          // should be sent out on the network, the global variable
          // uip_len is set to a value > 0.
          if (uip_len > 0)
          {
            tcpip_output();
            tcpip_output();
            B_(printf_small("ARP Package transmitted.\x0a\x0d");)
          }
        }
      }
    }
    /*
     * Schedule system tasks
     */
    PT_SCHEDULE(handle_sound(&sys_snd));
    PT_SCHEDULE(handle_led(&led));
    PT_SCHEDULE(handle_kicker(&kicker));
    PT_SCHEDULE(handle_time_client(&tc));
  }	// end of 'while (1)'
}

//-----------------------------------------------------------------------------
// Timer0_Init - sets up 10 mS RT interrupt
//-----------------------------------------------------------------------------
//
// Configure Timer0 to 16-bit generate an interrupt every 10 ms
//
void Timer0_Init (void)
{
#if BUILD_TARGET == IET912X
  SFRPAGE = TIMER01_PAGE;               // Set the correct SFR page
#endif
  TCON = 0x00;				                  // clear Timer0
//  TF0 = FALSE;				                  // clear overflow indicator
  CKCON = 0x00;			                    // Set T0 to SYSCLK/12
  TMOD = 0x01;				                  // TMOD: Clear
  TL0 = (-((SYSCLK/12)/100) & 0x00ff);  // Load timer 0 to give
  TH0 = (-((SYSCLK/12)/100) >> 8);      // 20MHz/12/100 = approx 10mS
  TR0 = TRUE;					                  // start Timer0
  PT0 = TRUE;					                  // T0 Interrupt Priority high
  ET0 = TRUE;					                  // enable Timer0 interrupts
#if BUILD_TARGET == IET912X
  SFRPAGE = LEGACY_PAGE;                // Reset to legacy SFR page
#endif
}

/* End of file */
