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
#define PRINT_A
//#define PRINT_B

#pragma codeseg APP_BANK

#include "system.h"       // System description
#include "main.h"         // Locals
#include "iet_debug.h"    // Debug macros
#include "swtimers.h"     // Software timer system
#include "flash.h"
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "pca.h"
#include "httpd-cgi.h"
#include "product.h"
#include "but_mon.h"
#include "ramp_mgr.h"
#include "absval_mgr.h"
#include "event_switch.h"
#include "adc_event.h"
#include "lightlib.h"
#include "demo.h"

extern static u16_t half_Sec;
extern static u16_t ten_Secs;
extern static near u8_t tmcnt;

extern bit TX_EventPending;	          // the DM9000 hardware receive event
extern bit ARP_EventPending;          // trigger the arp timer event
extern bit digit_select;
extern bit callback_kicker;

#define UPDATE_INTERVAL   25

void cleanup( void );
extern void config();
void Timer0_Init (void);

/*
 * Protothread instance data
 */
but_mon_t       but_mon;
ramp_mgr_t      ramp_mgr[4];
event_thread_t  event_thread;
absval_mgr_t    absval_mgr;
adc_event_t     adc_event;
rule_t          test_rule;
demo_t          demo;

// ---------------------------------------------------------------------------
//	pmd()
//
//	This is the PMD application
// ---------------------------------------------------------------------------
void pmd(void) banked
{
  unsigned int i;
  unsigned char update = UPDATE_INTERVAL;

  callback_kicker = 0;

  /* ***************** Initialize HW blocks ********************/
  config();                 // Configure the MCU

  half_Sec = UIP_TX_TIMER;
  ten_Secs = UIP_ARP_TIMER;

  Timer0_Init();            // 10 mSec interrupt rate

  sys_uart_init(BAUD_115200);  // Init the system print uart
  init_i2c();

  /* Initialize LED brightnes controller */
  init_pca(PCA_MODE_PWM_16, PCA_SYS_CLK);

  /* Initialize the ADC and ADC ISR */
  adc_init();

  /* ****************** Initialize libraries *******************/
  /* Initialise the uIP TCP/IP stack. */
  uip_init();

#ifdef HAVE_FLASH
  validate_config_flash();  // before we do anything else do this.
#endif

  init_swtimers();          // Initialize all software timers
  httpd_init();             // Initialise the webserver app.
  if (InitDM9000())         // Initialise the device driver.
    cleanup();              // exit if init failed
  uip_arp_init();           // Initialise the ARP cache.

  TX_EventPending = FALSE;	// False to poll the DM9000 receive hardware
  ARP_EventPending = FALSE;	// clear the arp timer event flag

  /* Initialize the LED pwm control library */
  init_ledlib();

  EA = TRUE;                // Enable interrupts

  A_(printf("\r\n");)
  printf("Invector Embedded Technologies Debug system output v. 1.001\r\n");
  printf("System: IET9123 mLight 4/2, 20MHz system clock, DM9000E Ethernet Controller\r\n");
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

  /* **********************Initialize system pthreads *******************/
  init_rtc();
  init_but_mon(&but_mon);

  /* Initialize all pwm ramp managers */
  for (i=0; i<CFG_NUM_PWM_DRIVERS; i++) {
    ramp_mgr[i].channel = i;
    init_ramp_mgr(&ramp_mgr[i]);
  }
  init_demo(&demo);
  init_event_switch(&event_thread);
  /* Initialize all event action managers before the event providers */
  /* Event action managers */
  init_absval_mgr(&absval_mgr);
  /* Event providers */
  init_adc_event(&adc_event);
  /* Here's a test rule */
  test_rule.base.type = EVENT_RULE;
  test_rule.base.name = "Test Rule";
  /* Just for testing, this will be the first event provider registered */
  test_rule.event = 0;
  /* Just for testing, this will be the first action manager registered */
  test_rule.action = 0;
  A_(printf(__FILE__ " Test rule ptr %p\n", &test_rule);)
  evnt_register_handle(&test_rule);

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
        C_(printf("Time for connection periodic management\x0a\x0d");)
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
        C_(printf("Time for udp periodic management\x0a\x0d");)
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
        B_(printf("ARP house keeping.\x0a\x0d");)
        ARP_EventPending = FALSE;
        uip_arp_timer();
      }
    }
    // Packet Received (uip_len != 0) Process incoming
    else
    {
      B_(printf("Received incomming data package.\x0a\x0d");)
      if (BUF->type == htons(UIP_ETHTYPE_IP))
      {
        B_(printf("IP Package received.\x0a\x0d");)
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
          B_(printf("IP Package transmitted.\x0a\x0d");)
        }
      }
      else
      {
        if (BUF->type == htons(UIP_ETHTYPE_ARP))
        {
          B_(printf("ARP package received.\x0a\x0d");)
          // Received an ARP packet
          uip_arp_arpin();
          // If the above function invocation resulted in data that
          // should be sent out on the network, the global variable
          // uip_len is set to a value > 0.
          if (uip_len > 0)
          {
            tcpip_output();
            tcpip_output();
            B_(printf("ARP Package transmitted.\x0a\x0d");)
          }
        }
      }
    }
    /*
     * Schedule system tasks
     */
    PT_SCHEDULE(handle_kicker(&kicker));
    PT_SCHEDULE(handle_time_client(&tc));
    PT_SCHEDULE(handle_but_mon(&but_mon));
    for (i=0; i<CFG_NUM_PWM_DRIVERS; i++) {
      PT_SCHEDULE(handle_ramp_mgr(&ramp_mgr[i]));
    }
    /* Event action managers */
    PT_SCHEDULE(handle_absval_mgr(&absval_mgr));
    /* Event providers */
    PT_SCHEDULE(handle_adc_event(&adc_event));
    PT_SCHEDULE(handle_event_switch(&event_thread));
    PT_SCHEDULE(handle_demo(&demo));

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
