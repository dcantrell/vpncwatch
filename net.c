/*
 * net.c
 * Network functions for vpncwatch
 * Author: David Cantrell <dcantrell@redhat.com>
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

int is_network_up(char *chkhost, unsigned short chkport) {
    int sock;
    struct sockaddr_in chksock;
    struct hostent *host = NULL;

    /* don't do a network check if the user didn't specify a host */
    if (chkhost == NULL || chkport == 0) {
        return 1;
    }

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        syslog(LOG_ERR, "socket() creation error: %s", strerror(errno));
        return 0;
    }

    memset(&chksock, 0, sizeof(chksock));
    chksock.sin_family = AF_INET;
    chksock.sin_port = htons(chkport);

    /* get the server address */
    if (inet_pton(AF_INET, chkhost, &(chksock.sin_addr.s_addr)) <= 0) {
        if ((host = gethostbyname(chkhost)) == NULL) {
            syslog(LOG_ERR, "%s", hstrerror(h_errno));
            return 0;
        }

        memcpy(&(chksock.sin_addr.s_addr), &(host->h_addr_list[0]),
               sizeof(struct in_addr));
    }

    /* try to connect */
    if (connect(sock, (struct sockaddr *) &chksock, sizeof(chksock)) < 0) {
        syslog(LOG_ERR, "connect() failed: %s", strerror(errno));
        return 0;
    }

    close(sock);
    return 1;
}
