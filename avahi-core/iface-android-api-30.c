/***
  This file is part of avahi.

  avahi is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  avahi is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
  Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with avahi; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <string.h>
#include <net/if.h>
#include <linux/if_addr.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <avahi-common/malloc.h>
#include "iface.h"
#include "addr-util.h"

int avahi_interface_monitor_init_osdep(AvahiInterfaceMonitor *m) {
    AvahiHwInterface *hw;
    AvahiInterface *i;
    AvahiAddress raddr;
    AvahiInterfaceAddress *addr;
    struct ifaddrs *addrs_list;
    struct ifaddrs *ifa;

    getifaddrs(&addrs_list);
    for (ifa = addrs_list; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || (ifa->ifa_addr->sa_family != AF_INET && ifa->ifa_addr->sa_family != AF_INET6))
            continue;

        unsigned int ifi_index = if_nametoindex(ifa->ifa_name);
        if (!(hw = avahi_interface_monitor_get_hw_interface(m, ifi_index))) {

            /* No object found, so let's create a new
             * one. avahi_hw_interface_new() will call
             * avahi_interface_new() internally twice for IPv4 and
             * IPv6, so there is no need for us to do that
             * ourselves */
            if ((hw = avahi_hw_interface_new(m, (AvahiIfIndex) ifi_index))) {
                hw->flags_ok =
                        (ifa->ifa_flags & IFF_UP) &&
                        (!m->server->config.use_iff_running || (ifa->ifa_flags & IFF_RUNNING)) &&
                        !(ifa->ifa_flags & IFF_LOOPBACK) &&
                        (ifa->ifa_flags & IFF_MULTICAST) &&
                        (m->server->config.allow_point_to_point || !(ifa->ifa_flags & IFF_POINTOPOINT));

                avahi_free(hw->name);
                hw->name = avahi_strndup(ifa->ifa_name, strlen(ifa->ifa_name));

// fixme - these mtu values are differnet from what ip addr reports
//				struct ifreq ifr;
//				memset(&ifr, 0, sizeof(ifr));
//				int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
//				strcpy(ifr.ifr_name, hw->name);
//				if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
//					//perror("SIOCGIFFLAGS");
//				}
//				close(sock);
//				hw->mtu = ifr.ifr_mtu;

                /* Check whether this interface is now "relevant" for us. If
                 * it is Avahi will start to announce its records on this
                 * interface and send out queries for subscribed records on
                 * it */
                avahi_hw_interface_check_relevant(hw);

                /* Update any associated RRs of this interface. (i.e. the
                 * _workstation._tcp record containing the MAC address) */
                avahi_hw_interface_update_rrs(hw, 0);
            } else {
                goto out;
            }
        }

        /* Try to get a reference to our AvahiInterface object for the
         * interface this address is assigned to. If ther is no object
         * for this interface, we ignore this address. */
        if (!(i = avahi_interface_monitor_get_interface(m, (AvahiIfIndex) ifi_index, avahi_af_to_proto(ifa->ifa_addr->sa_family))))
            continue;

        avahi_address_from_sockaddr(ifa->ifa_addr, &raddr);

        /* This address is new or has been modified, so let's get an object for it */
        if (!(addr = avahi_interface_monitor_get_address(m, i, &raddr)))

            /* Mmm, no object existing yet, so let's create a new one */
            if (!(addr = avahi_interface_address_new(m, i, &raddr, 4)))
                continue; /* OOM */

        /* Update the scope field for the address */
        //addr->global_scope = 0;//ifaddrmsg->ifa_scope == RT_SCOPE_UNIVERSE || ifaddrmsg->ifa_scope == RT_SCOPE_SITE;
        addr->deprecated = !!(ifa->ifa_flags & IFA_F_DEPRECATED);

        /* Avahi only considers interfaces with at least one address
         * attached relevant. Since we migh have added or removed an
         * address, let's have it check again whether the interface is
         * now relevant */
        avahi_interface_check_relevant(i);

        /* Update any associated RRs, like A or AAAA for our new/removed address */
        avahi_interface_update_rrs(i, 0);
    }

    m->list_complete = 1;
    avahi_interface_monitor_check_relevant(m);

    /* And update all RRs attached to any interface */
    avahi_interface_monitor_update_rrs(m, 0);

out:
    freeifaddrs(addrs_list);

    return 0;
}

void avahi_interface_monitor_free_osdep(AvahiInterfaceMonitor *m) {
    (void) m;
}

void avahi_interface_monitor_sync(AvahiInterfaceMonitor *m) {
    (void) m;
}
