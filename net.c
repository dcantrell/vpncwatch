/*
 * net.c
 * Network functions for vpncwatch
 * Author: David Cantrell <david.l.cantrell@gmail.com>
 *
 * Adapted from vpnc-watch.py by Gary Benson <gbenson@redhat.com>
 * (Python is TOO BIG for a 16M OpenWRT router.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "vpncwatch.h"

int is_network_up(char *chkhost, char *chkport) {
    int sfd, s;
    struct addrinfo *result, *rp;
    struct addrinfo hints;

    /* don't do a network check if the user didn't specify a host */
    if (chkhost == NULL || chkport == NULL) {
        return 1;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    /* get the server address */
    if ((s = getaddrinfo(chkhost, chkport, &hints, &result)) != 0) {
        syslog(LOG_ERR, "Error getting addr info: %s", gai_strerror(s));
    }

    /* try list of addresses returned by getaddrinfo */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    freeaddrinfo(result);           /* No longer needed */
    if (rp == NULL) {               /* No address succeeded */
        syslog(LOG_ERR, "Could not connect");
        return 0;
    }

    return 1;
}
