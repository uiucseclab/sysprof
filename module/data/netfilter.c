/*
 * netfilter.c - Collect data about packets and network operations
 *
 * Copyright (C) 2016 Quytelda Kahja, Roger Xiao
 * This file is part of Sysprof.
 *
 * Sysprof is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Sysprof is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Sysprof.	If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <stats.h>

static unsigned int udp_in_count = 0, tcp_in_count = 0, icmp_in_count = 0;
static unsigned int udp_out_count = 0, tcp_out_count = 0, icmp_out_count = 0;
static unsigned int packets_in_count = 0, packets_out_count = 0;

ssize_t netfilter_report(void ** data)
{
    struct nf_data nfdata =
    {
	.pac_in  = packets_in_count,
	.pac_out = packets_out_count,
    };

    *data = (void *) &nfdata;
    return sizeof(struct nf_data);
}

static unsigned int hook_func_in(void * priv,
				 struct sk_buff * skb,
				 const struct nf_hook_state * state)
{
    packets_in_count++; // keep track of incoming packet count

    /* TODO: we can find out it the packet is ipv4 or ipv6:
     *     nf_ct_l3num(skb->nfct)
     */

    // get the protocol, length, source IP, and destination IP of a packet caught in the hook:
    struct iphdr * ip_header = (struct iphdr *) skb_network_header(skb);

    /* TODO: the iphdr also has parameters check, frag_off, id, tot_len, and
     * ttl. What do they do? How can we use them?  Anything we can learn
     * from ttl (time to live) value? */
    unsigned int len = (unsigned int) ip_header->tot_len;
    unsigned int src_ip  = (unsigned int) ip_header->saddr;
    unsigned int dest_ip = (unsigned int) ip_header->daddr;
    unsigned int src_port = 0, dest_port = 0;

    switch(ip_header->protocol)
    {
    case IPPROTO_UDP :
	udp_in_count++;

	struct udphdr * udp_header = (struct udphdr *) skb_transport_header(skb);
	src_port  = (unsigned int) ntohs(udp_header->source);
	dest_port = (unsigned int) ntohs(udp_header->dest);

	src_ip  = (unsigned int) udp_header->source;
	dest_ip = (unsigned int) udp_header->dest;

	break;
    case IPPROTO_TCP :
	tcp_in_count++;

	struct tcphdr * tcp_header = (struct tcphdr *) skb_transport_header(skb);
	src_port  = (unsigned int) ntohs(tcp_header->source);
	dest_port = (unsigned int) ntohs(tcp_header->dest);

	src_ip  = (unsigned int) tcp_header->source;
	dest_ip = (unsigned int) tcp_header->dest;

	break;
    case IPPROTO_ICMP : 
	icmp_in_count++;
	/* XXX: ICMP header does not contain source/destination information. */
	break;
    default : // no special treatment
	src_ip  = (unsigned int) ip_header->saddr;
	dest_ip = (unsigned int) ip_header->daddr;
    }

    /* TODO: switch(dest_ip)
    {
	//Keep track of where the packets are going to. If we send an anomalous amount of packets to a bunch of very distant
	//addresses, something might be wrong.
	//Sending to a lot of IPs might be more suspicious than sending to very few--A chat session with another machine
	//may cause a lot of traffic to a specific IP address, but there is little reason to send a single packet to 10,000
	//different IP addresses
    }*/

    // TODO: send counts to shared buffer?

    // let the packet through. We're just observing
    return NF_ACCEPT;
}

static unsigned int hook_func_out(void * priv,
				 struct sk_buff * skb,
				 const struct nf_hook_state * state)
{
    packets_out_count++; // keep track of incoming packet count

    // get the protocol, length, source IP, and destination IP of a packet caught in the hook:
    struct iphdr * ip_header = (struct iphdr *) skb_network_header(skb);

    /* TODO: the iphdr also has parameters check, frag_off, id, tot_len, and
     * ttl. What do they do? How can we use them?  Anything we can learn
     * from ttl (time to live) value? */
    unsigned int len = (unsigned int) ip_header->tot_len;
    unsigned int src_ip  = (unsigned int) ip_header->saddr;
    unsigned int dest_ip = (unsigned int) ip_header->daddr;
    unsigned int src_port = 0, dest_port = 0;

    switch(ip_header->protocol)
    {
    case IPPROTO_UDP :
	udp_out_count++;

	struct udphdr * udp_header = (struct udphdr *) skb_transport_header(skb);
	src_port  = (unsigned int) ntohs(udp_header->source);
	dest_port = (unsigned int) ntohs(udp_header->dest);

	src_ip  = (unsigned int) udp_header->source;
	dest_ip = (unsigned int) udp_header->dest;

	break;
    case IPPROTO_TCP :
	tcp_out_count++;

	struct tcphdr * tcp_header = (struct tcphdr *) skb_transport_header(skb);
	src_port  = (unsigned int) ntohs(tcp_header->source);
	dest_port = (unsigned int) ntohs(tcp_header->dest);

	src_ip  = (unsigned int) tcp_header->source;
	dest_ip = (unsigned int) tcp_header->dest;

	break;
    case IPPROTO_ICMP :
	icmp_out_count++;
	break;
    }

    // let the packet through. We're just observing
    return NF_ACCEPT;
}

static struct nf_hook_ops nfho_in =
{
    .hook     = hook_func_in,
    .hooknum  = NF_INET_LOCAL_IN,
    .pf	      = PF_INET,
    .priority = NF_IP_PRI_FIRST,
};

static struct nf_hook_ops nfho_out =
{
    .hook	= hook_func_out,
    .hooknum	= NF_INET_LOCAL_OUT,
    .pf		= PF_INET,
    .priority	= NF_IP_PRI_FIRST, // different hook, can also be first
};

//Via example at http://pleviaka.pp.fi/netfilter_module.c

/*
 */
void init_netfilter(void)
{
    nf_register_hook(&nfho_in);	     // Register the hook
    nf_register_hook(&nfho_out);
}


void exit_netfilter(void)
{
    nf_unregister_hook(&nfho_in);
    nf_unregister_hook(&nfho_out);
}
