/*
 * vpncwatch.h
 * Keepalive daemon for vpnc (so I can fit it on an OpenWRT system)
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

/* Program version */
#define VER_MAJOR 1
#define VER_MINOR 8

/* Prototypes */
void show_version(char *);
void show_usage(char *);
char *which(char *);
pid_t pidof(char *);
int is_running(int);
void signal_handler(int);
pid_t start_cmd(char *, char *, char **);
void stop_cmd(char *, int);
int is_network_up(char *, unsigned short);
