/*
 * vpncwatch.c
 * Keepalive daemon for vpnc (so I can fit it on an OpenWRT system)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>

#include "vpncwatch.h"

extern char *optarg;
extern int optind, opterr, optopt;

sig_atomic_t do_restart = 0;
sig_atomic_t do_exit = 0;

/* Display program version */
void show_version(char *prog) {
    printf("%s-%d.%d\n", basename(prog), VER_MAJOR, VER_MINOR);
    return;
}

/* Display simple usage screen */
void show_usage(char *prog) {
    prog = basename(prog);

    if (prog == NULL) {
        prog = "vpncwatch";
    }

    show_version(prog);
    printf("Usage: %s [options] <vpnc executable> <vpnc args>\n\n", prog);
    printf("Options:\n");
    printf("    -c HOST    Hostname or IP address on the VPN to check ");
    printf("periodically for\n");
    printf("               connectivity.  If the test fails, ");
    printf("vpnc will be restarted.\n");
    printf("    -p PORT    TCP port number to test on HOST.\n");
    printf("    -i SECS    Interval in seconds between VPN host check ");
    printf("(default: 3600).\n");
    printf("    -?         Show this screen.\n");
    printf("    -V         Show version.\n\n");
    printf("Examples:\n");
    printf("    %s vpnc /etc/vpnc/vpnc.conf\n", prog);
    printf("    %s /usr/etc/vpnc --gateway 1.2.3.4 --id ID\n", prog);
    printf("    %s -c intranet.corp.redhat.com -p 80 vpnc /etc/vpnc/vpnc.conf\n\n", prog);
    printf("See the man page for vpnc(8) for more information.  Please note that\n");
    printf("the vpnc executable and vpnc argument list need be at the end of the\n");
    printf("%s command line.\n", prog);
    return;
}

/* signal handler */
void signal_handler(int sig) {
    if (sig == SIGHUP) {
        do_restart = 1;
    } else if (sig == SIGTERM) {
        do_exit = 1;
    }

    return;
}

int main(int argc, char **argv) {
    int c, running;
    char *me = NULL, *cmd = NULL, *cmdpath = NULL;
    char cmdbuf[PATH_MAX];
    pid_t cmdpid = 0;
    char *chkhost = NULL;
    unsigned short chkport = 0;
    unsigned int chkinterval = 3600;

    /* handle options */
    if (argc < 2) {
        show_usage(argv[0]);
        return EXIT_FAILURE;
    }

    while ((c = getopt(argc, argv, "c:p:i:?V")) != -1) {
        switch (c) {
            case 'c':
                chkhost = strdup(optarg);
                break;
            case 'p':
                errno = 0;
                chkport = strtol(optarg, NULL, 10);

                if (((chkport == LONG_MIN || chkport == LONG_MAX) &&
                     (errno == ERANGE)) ||
                    ((chkport == 0) && (errno == EINVAL))) {
                    fprintf(stderr, "%s (%d): %s", __func__, __LINE__,
                            strerror(errno));
                    fflush(stderr);
                    show_usage(argv[0]);
                    return EXIT_FAILURE;
                }

                if ((chkport <= 0) || (chkport >= 65536)) {
                    fprintf(stderr, "Error: Invalid port specified: %d\n",
                            chkport);
                    fflush(stderr);
                    show_usage(argv[0]);
                    return EXIT_FAILURE;
                }

                break;
            case 'i':
                errno = 0;
                chkinterval = strtol(optarg, NULL, 10);

                if (((chkinterval == LONG_MIN || chkinterval == LONG_MAX) &&
                     (errno == ERANGE)) ||
                    ((chkinterval == 0) && (errno == EINVAL))) {
                    fprintf(stderr, "%s (%d): %s", __func__, __LINE__,
                            strerror(errno));
                    fflush(stderr);
                    show_usage(argv[0]);
                    return EXIT_FAILURE;
                }

                if (chkinterval < 0) {
                    fprintf(stderr, "Error: Time interval must be >0\n");
                    fflush(stderr);
                    show_usage(argv[0]);
                    return EXIT_FAILURE;
                }

                break;
            case '?':
                show_usage(argv[0]);
                return EXIT_SUCCESS;
            case 'V':
                show_version(argv[0]);
                return EXIT_SUCCESS;
            default:
                show_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    /* set up control variables */
    me = basename(argv[0]);
    cmd = basename(argv[1]);
    argv[1] = strdup(cmd);
    argv += optind;                 /* skip to the end.... */

    if ((cmdpath = realpath(which(cmd), cmdbuf)) == NULL) {
        syslog(LOG_ERR, "realpath failure: %s:%d", __func__, __LINE__);
        return EXIT_FAILURE;
    }

    /* create a syslog interface */
    openlog(me, LOG_CONS | LOG_PID, LOG_DAEMON);

    /* see if we are already running */
    if ((cmdpid = pidof(cmd)) != 0) {
        syslog(LOG_ERR, "%s already running (%d)", cmd, cmdpid);
        return EXIT_FAILURE;
    }

    /* start the command */
    if ((cmdpid = start_cmd(cmd, cmdpath, argv)) == -1) {
        syslog(LOG_ERR, "start_cmd failure");
        return EXIT_FAILURE;
    }

    syslog(LOG_ERR, "cmdpid: |%d|", cmdpid);

    /* run in the background */
    if (daemon(0, 0) == -1) {
        syslog(LOG_ERR, "daemon failure: %s:%d", __func__, __LINE__);
        return EXIT_FAILURE;
    }

    /* install our signal handler */
    if (signal(SIGHUP, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "signal failure: %s:%d", __func__, __LINE__);
        return EXIT_FAILURE;
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "signal failure: %s:%d", __func__, __LINE__);
        return EXIT_FAILURE;
    }

    /* make sure we have a check interval */
    if (chkinterval < 1) {
        chkinterval = 3600;
    }

    /* main running loop */
    do_exit = 0;
    while (!do_exit) {
        do_restart = 0;
        sleep(1);
        running = is_running(cmdpid);

        /* see if vpnc is running */
        if (!running) {
            syslog(LOG_WARNING, "%s died", cmd);
        } else if (do_exit || do_restart) {
            stop_cmd(cmd, cmdpid);
        }

        /* assuming it's running, check the actual network link */
        if (running && chkhost != NULL && chkport > 0) {
            if (!is_network_up(chkhost, chkport)) {
                do_restart = 1;
            }
        }

        if (do_restart || !running) {
            cmdpid = start_cmd(cmd, cmdpath, argv);
        }
    }

    syslog(LOG_INFO, "exiting");
    return EXIT_SUCCESS;
}
