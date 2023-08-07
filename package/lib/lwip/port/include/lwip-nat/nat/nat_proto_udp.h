/*
 * Copyright (c) 2018 Russ Dill <russ.dill@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#ifndef __NAT_NAT_PROTO_UDP_H__
#define __NAT_NAT_PROTO_UDP_H__

#include <lwip/ip_addr.h>

struct udp_hdr;
struct netif;
struct nat_pcb;

void nat_udp_expire(void);

struct nat_pcb *icmp_udp_prerouting_pcb(const ip_addr_t *iphdr_src,
			const ip_addr_t *iphdr_dest, struct pbuf *p,
			struct netif *inp, struct netif *forwardp);
void icmp_udp_prerouting_nat(u16_t *icmp_chksum, const ip_addr_t *iphdr_src,
			const ip_addr_t *iphdr_dest, struct pbuf *p,
			struct nat_pcb *pcb, int forward);

struct nat_pcb *udp_prerouting_pcb(struct pbuf *p, struct netif *inp,
			struct netif *forwardp);
void udp_prerouting_nat(struct pbuf *p, struct nat_pcb *pcb, int forward);

#endif
