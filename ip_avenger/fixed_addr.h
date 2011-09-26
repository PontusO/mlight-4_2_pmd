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
#ifndef FIXED_ADDR_H_INCLUDED
#define FIXED_ADDR_H_INCLUDED

#define UIP_IPADDR0     192 /**< The first octet of the IP address of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_IPADDR1     168 /**< The second octet of the IP address of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_IPADDR2     0   /**< The third octet of the IP address of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_IPADDR3     10   /**< The fourth octet of the IP address of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */

#define UIP_NETMASK0    255 /**< The first octet of the netmask of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_NETMASK1    255 /**< The second octet of the netmask of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_NETMASK2    255 /**< The third octet of the netmask of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_NETMASK3    0   /**< The fourth octet of the netmask of
			       this uIP node, if UIP_FIXEDADDR is
			       1. \hideinitializer */

#define UIP_DRIPADDR0   192 /**< The first octet of the IP address of
			       the default router, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_DRIPADDR1   168 /**< The second octet of the IP address of
			       the default router, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_DRIPADDR2   0   /**< The third octet of the IP address of
			       the default router, if UIP_FIXEDADDR is
			       1. \hideinitializer */
#define UIP_DRIPADDR3   1   /**< The fourth octet of the IP address of
			       the default router, if UIP_FIXEDADDR is
			       1. \hideinitializer */

#define UIP_ETHADDR0    0x00  /**< The first octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR1    0x1d  /**< The second octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR2    0x3b  /**< The third octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR3    0x33  /**< The fourth octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR4    0x05  /**< The fifth octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */
#define UIP_ETHADDR5    0x72  /**< The sixth octet of the Ethernet
				 address if UIP_FIXEDETHADDR is
				 1. \hideinitializer */

#endif // FIXED_ADDR_H_INCLUDED
