#include <runtime/lib.h>
#include <kernel/uos.h>
#include <mem/mem.h>
#include <buf/buf.h>
#include <net/netif.h>
#include <net/arp.h>

bool_t
netif_output_prio (netif_t *netif, buf_t *p
        , ip_addr_const ipdest
        , ip_addr_const ipsrc
        , small_uint_t prio)
{
	if (netif->arp && ipadr_not0(ipsrc)) {	/* vch: для бриджуемых фреймов не нужен arp */
		unsigned char *ethdest = 0;

		/* For broadcasts, ipdest must be NULL. */
		if (ipadr_not0(ipdest) && (ipref_as_ucs(ipdest)[0] & 0xf0) != 0xe0) {
			/* Search the ARP table for MAC address. */
			ethdest = arp_lookup (netif, ipdest);
			if (! ethdest) {
				/* Send ARP request. */
				arp_request (netif, p, ipdest, ipsrc);
				goto discard;
			}
		}
		if (! arp_add_header (netif, p, ipref_as_ucs(ipdest), ethdest)) {
discard:		/* Count this packet as discarded. */
			++netif->out_discards;
			return 0;
		}
	}
	return netif->interface->output (netif, p, prio);
}

bool_t
netif_output (netif_t *netif, buf_t *p
        , ip_addr_const ipdest
        , ip_addr_const ipsrc)
{
	return netif_output_prio (netif, p, ipdest, ipsrc, 0);
}

buf_t *
netif_input (netif_t *netif)
{
	struct _buf_t *p;

	p = netif->interface->input (netif);
	if (p && netif->arp)
		p = arp_input (netif, p);
	return p;
}

void
netif_set_address (netif_t *netif, unsigned char *ethaddr)
{
	if (netif->interface->set_address)
		netif->interface->set_address (netif, ethaddr);
}
