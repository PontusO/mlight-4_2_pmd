/*
 * Copyright (c) 2006, Invector Embedded Technologies
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
 * This file is part of the uIP TCP/IP stack.
 *
 * Author:    Tony Pemberton, Pontus Oldberg
 *
 */

// ---------------------------------------------------------------------------
//
// Module:			DM9000 ethernet routines for 8051 and SDCC port of Adam Dunkels'
//              uIP v0.9
// Filename:		DM9000.c
// Version:			0.1
// Date:			  30/MAR/2005
// Author:			Adam Dunkels, A.Pemberton, Pontus Oldberg
// Function:		DM9000 and other H/W definitions
//
// Change History:
//              2006-03-20 PO Added critical sections directive to functions
//              2006-03-15 PO Cleaned up code
//              2005-09-14 AP Basic DM9000 module - frills removed
//
//
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "dm9000.h"		  // Physical Layer

//---------------------------------------------------------------------------------
// Static variables for DM9000
//---------------------------------------------------------------------------------

extern __bit TX_EventPending;	// the DM9000 hardware receive event
extern __bit ARP_EventPending;	// trigger the arp timer event

static __xdata u16_t RX_Length;	// Physical size of packet must be completely read!

//---------------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Write Command to NIC
//-----------------------------------------------------------------------------

void write_nicreg(u8_t IOaddr, u8_t val)
{
  ENTER_CRITICAL_SECTION;

  NIC_REG = IOaddr;
  NIC_DAT = val;

  EXIT_CRITICAL_SECTION;
}

//-----------------------------------------------------------------------------
// Write Data to NIC
//-----------------------------------------------------------------------------
void write_nicdata(u8_t val)
{
  NIC_DAT = val;
}

//-----------------------------------------------------------------------------
// Write Command or Data to already selected register
//-----------------------------------------------------------------------------
void set_nicreg(u8_t IOaddr)
{
  NIC_REG = IOaddr;
}

//-----------------------------------------------------------------------------
// Write Command to PHY
//-----------------------------------------------------------------------------

void PHY_write(u8_t IOaddr, u16_t val)
{
  ENTER_CRITICAL_SECTION;

  write_nicreg(DM9000_EPCR,0x0a);					// Set EEPROM & PHY Control Reg to write
  write_nicreg(DM9000_EPAR,((IOaddr & 0x3f) | DM9000_PHY));	// Set PHY Address Reg to PHY Address
  write_nicreg(DM9000_EPDRH,(u8_t)(val >> 8));	// write high_byte of PHY data
  write_nicreg(DM9000_EPDRL,(u8_t)(val));			// write low_byte of PHY data
  write_nicreg(DM9000_EPCR,0x00);					// Clear EEPCR

  EXIT_CRITICAL_SECTION;
}

//--------------------------------------------------------------------------------
// Read Status from NIC
//--------------------------------------------------------------------------------
u8_t read_nicreg(u8_t IOaddr)
{
  u8_t temp;

  ENTER_CRITICAL_SECTION;

  NIC_REG = IOaddr;
  temp = NIC_DAT;

  EXIT_CRITICAL_SECTION;

  return temp;

}

//--------------------------------------------------------------------------------
// Read Data from NIC from preselected address
//--------------------------------------------------------------------------------
u8_t read_nicdata(void)
{
  return (NIC_DAT);
}

//-----------------------------------------------------------------------------
// Non-__interrupt delay approx 1mS to 255mS
//-----------------------------------------------------------------------------
static void _wait_ms(u16_t count)
{
  __near u16_t i;

  for ( ; count > 0; count--) {
    for ( i = 10000; i > 0; i--);
  }
}

//-----------------------------------------------------------------------------
// u8_t InitDM9000(void);
//
// Init the PHY. If OK, return true
//-----------------------------------------------------------------------------
// Driver Initializing Steps

u8_t InitDM9000(void)
{

  //1. If the internal PHY is required, the following steps are to active the internal PHY:
  // The default status of the DM9000 is to power down the internal PHY by setting the GPIO0.
  // Since the internal PHY have been powered down, the wakeup procedure will be needed to
  // enable the DM9000.
  write_nicreg(DM9000_GPCR, 0x07);    // Power PHY set the GPR to output
  write_nicreg(DM9000_GPR,  0x06);	  // Clear the GPIO lines (Led's off)
  _wait_ms(1);

  //2. Program NCR register. Choose normal mode by setting NCR (reg_00h) __bit[2:1] = 00h. The
  // system designer can choose the network operation such as setting internal/external PHY,
  // enable wakeup event or choose the full-duplex mode. Please refer to the DM9000 datasheet
  // ch.6.1 about NCR register setting.
  // Reset DM9000 (twice)
  write_nicreg(DM9000_NCR, 0x03);	// Reset
  _wait_ms(1);
  write_nicreg(DM9000_NCR, 0x00);	// Normal mode
  _wait_ms(1);

  write_nicreg(DM9000_NCR, 0x03);
  _wait_ms(1);
  write_nicreg(DM9000_NCR, 0x00);
  _wait_ms(1);

  //3. Clear TX status by reading NSR register (reg_01h). __bit[2:3] and __bit[5] will be automatically
  // cleared by reading or writing 1. Please refer to the DM9000 datasheet ch.6.2 about NSR
  // register setting.
  read_nicreg(DM9000_NSR);

  //4. Read EEPROM (No EEPROM available - use scratchpad instead) data if
  // valid or required. By default, the array will have hard MAC loaded.

  //5. Set Node address 6 bytes from in physical address register (reg_10h~15h).
  write_nicreg(DM9000_MAC_REG,  uip_ethaddr.addr[0]);
  write_nicreg(DM9000_MAC_REG+1,uip_ethaddr.addr[1]);
  write_nicreg(DM9000_MAC_REG+2,uip_ethaddr.addr[2]);
  write_nicreg(DM9000_MAC_REG+3,uip_ethaddr.addr[3]);
  write_nicreg(DM9000_MAC_REG+4,uip_ethaddr.addr[4]);
  write_nicreg(DM9000_MAC_REG+5,uip_ethaddr.addr[5]);

  //6. Set Hash table 8 bytes from multicast address register (reg_16h~1Dh).
  write_nicreg(DM9000_MLC_REG,  0xff);
  write_nicreg(DM9000_MLC_REG+1,0xff);
  write_nicreg(DM9000_MLC_REG+2,0xff);
  write_nicreg(DM9000_MLC_REG+3,0xff);
  write_nicreg(DM9000_MLC_REG+4,0xff);
  write_nicreg(DM9000_MLC_REG+5,0xff);
  write_nicreg(DM9000_MLC_REG+6,0xff);
  write_nicreg(DM9000_MLC_REG+7,0xff);

  //7. reset Internal PHY if desired
  //	PHY_write(DM9000_BMCR, 0x8000);	// PHY reset
  //	 _wait_ms(1000);
  //	PHY_write(DM9000_BMCR, 0x1200);	// Auto negotiation
  //	 _wait_ms(1000);


  //8. Set IMR register (reg_FFh) __bit[7]=1 to enable the SRAM read/write pointer which is the
  // automatic return function of the memory R/W address pointer; also set the receive int mask.
  write_nicreg(DM9000_IMR, 0x81);

  //9. Depend on OS and DDK of the system to handle NIC interrupts.
  // Not applicable since the DM9000 is polled

  //10. Program IMR register (reg_FFh) __bit[1:0] to enable the TX/RX __interrupt. Before doing this,
  // the system designer needs to register the __interrupt handler routine. For example, if the driver
  // needs to generate the __interrupt after a package is transmitted, the __interrupt mask register IMR
  // __bit[1]=1 will be set. If the __interrupt is generated by the DM9000 after receiving a packet, IMR
  // __bit[0] should be set to 1.
  // Not applicable since the DM9000 is polled

  //11. Program RXCR register to enable RX. The RX function is enabled by setting the RX control
  // register (reg_05h) __bit[0]=1. The choice of the other bits __bit[6:0] depends on the system design.
  // Please refer to the DM9000 datasheet ch.6.6 about RXCR register setting.
  write_nicreg(DM9000_RXCR, 0x31);	// Only MAC-packets and RX enable - no multicast packets

  //12. NIC is being activated now.

  //13. Check Vendor ID is Davicom (0x0A46)
  if ((read_nicreg(DM9000_VID) + (read_nicreg(DM9000_VID+1) << 8)) != DM9000_VendID)
    return TRUE;		// No chip found

  if ((read_nicreg(DM9000_PID) + (read_nicreg(DM9000_PID+1) << 8)) != DM9000_ProdID)
    return TRUE;		// No chip found

  RX_Length = 0;

  return FALSE;
}
// ---------------------------------------------------------------------------
//							DM9000_receive()
//
//	This function will read an entire IP packet into the uip_buf.
//	If it must wait for more than 0.5 seconds, it will return with
//	the return value 0. Otherwise, when a full packet has been read
//	into the uip_buf buffer, the length of the packet is returned.
//
// Checks Physical layer for incoming Data, if applicable, forewind
// to 1st IP-Byte
// (for DM9000: Drop Status, but keep and (Physical) length)
// ---------------------------------------------------------------------------
u16_t DM9000_receive(void)
{

  __idata u8_t RX_ready;
  __idata u16_t RX_status;
  __idata int i;

  // read the packet ready flag
  // Check if __bit[0]=PRS=1, RX OK, a frame has been received
  read_nicreg(DM9000_MRCMDX); // Read the packet ready flag

  if ((RX_ready = read_nicdata()) & DM9000_PKT_RDY) // ready check: this byte must be 0 or 1
  {

    // get (and check) the RX status and get RX length
    // Set memory read autoincrement pointer
    set_nicreg(DM9000_MRCMD);

    // Now read the two first bytes of the packet
    // First byte is RX packet present - should be 0x01. If 0x00 then no packet follows. Otherwise it is an error
    RX_status = (read_nicdata() + (read_nicdata() << 8));

    // Now get the packet data length which will not include these four bytes
    RX_Length = (read_nicdata() + (read_nicdata() << 8));

    // Now read the RX packet data to the previously recorded length
    for (i = 0; i < RX_Length; i++)
    {
      *(uip_buf + i) = read_nicdata();
    }
    write_nicreg(DM9000_ISR, DM9000_RX_INTR);// clear __bit[0]=PRS latched in ISR
    return RX_Length;// Frame received - return number of bytes in frame
  }
  // If rx_ready is 0, a frame error has occurred - reset the PHY
  else if (RX_ready != 0x00)
  {
    write_nicreg(DM9000_IMR, 0x80);   // Stop __interrupt Request
    write_nicreg(DM9000_ISR, 0x0F);   // Clear __interrupt Status
    write_nicreg(DM9000_RXCR, 0x00);  // Stop Receive function

    InitDM9000();			                // Error occured! Reset PHY
    return 0;                         // error
  }
  write_nicreg(DM9000_ISR, DM9000_RX_INTR);// clear __bit[0]=PRS latched in ISR

  return 0;		// No Frame received
}

//-----------------------------------------------------------------------------
// 					DM9000_transmit(void)
//
// Send the packet in the uip_buf and uip_appdata buffers
// __using the DM9000 ethernet chip.
// The uIP packet buffer.
//
// The uip_buf array is used to hold incoming and outgoing packets.
// The device driver should place incoming data into this buffer.
// When sending data, the device driver should read the
// link level headers and the TCP/IP headers from this buffer.
// The size of the link level headers is configured by the
// UIP_LLH_LEN define.
//
//-----------------------------------------------------------------------------
void tcpip_output(void)
{
  DM9000_transmit();
}
void DM9000_transmit(void)
{
  __idata u16_t i;
  __idata u16_t j;
  u8_t *ptr; // general pointer for data

  //Step 1: Wait for current transmit to complete
  while (read_nicreg(DM9000_TCR) & 0x01);

  // while (read_nicreg(DM9000_NSR) & (4 | 8));

  write_nicreg(DM9000_ISR, DM9000_TX_INTR);

  //Step 2: write the data into TX SRAM byte mode with address autoincrement
  set_nicreg(DM9000_MWCMD);

  // Transfer header info to SRAM
  // 54 octets - header, etc. info
  ptr = &uip_buf[0];

  // Modified to allow for the TCP options field length if supplied
  // uip_buf[UIP_LLH_LEN + 9] is the header protocol octet
  // uip_buf[UIP_LLH_LEN + 32] is the TCP offset octet which determines where the TCP header ends and data begins

  if (uip_buf[UIP_LLH_LEN + 9] == UIP_PROTO_TCP)
  {
    j = (UIP_LLH_LEN + UIP_TCPIP_HLEN + (((uip_buf[UIP_LLH_LEN + 32] >> 4) - 5) << 2));
  }
  else if (uip_buf[UIP_LLH_LEN + 9] == UIP_PROTO_ICMP)
  {
    j = (UIP_LLH_LEN + UIP_TCPIP_HLEN);
  }
  else if (uip_buf[UIP_LLH_LEN + 9] == UIP_PROTO_UDP)
  {
    j = (UIP_LLH_LEN + UIP_TCPIP_HLEN);
  }
  else
    j = (UIP_LLH_LEN + UIP_TCPIP_HLEN);

  for (i = 0; i < j; i++)
  {
    write_nicdata(*ptr++); // write header data to transmit SRAM
  }

  // reached the end of header, switch pointer to uip_appdata to be sent.
  // Write appdata to DM9000 - should include any offset
  if (i < uip_len)
  {
    for (ptr = uip_appdata; (i < uip_len) ; i++)
    {
      write_nicdata(*ptr++); // write data to transmit SRAM
    }
  }

  //Step 3: write the xmit  data length into MDRAL (reg_FCh) & MDRAH (reg_FDh)
  // write high_byte of the transmitted data length into MDRAH (reg_FDh)
  write_nicreg(DM9000_TXPLH,(u8_t)(i >> 8));
  // write low_byte of the transmitted data length into MDRAL (reg_FCh)
  write_nicreg(DM9000_TXPLL,(u8_t)(i & 0xff));

  //Step 4: start to transmit a packet
  // Issue command for DM9000 to transmit packet from TX SRAM.
  write_nicreg(DM9000_TCR, 1);
}

//-----------------------------------------------------------------------------
// END
