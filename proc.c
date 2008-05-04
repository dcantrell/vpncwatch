/*
 * proc.c
 * Process control functions for vpncwatch
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
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#include "vpncwatch.h"

/* Implements which(1) as a function */
char *which(char *cmd) {
    char *dir = NULL, *path = NULL, *ret = NULL;

    if (cmd == NULL) {
        syslog(LOG_ERR, "input to which() is NULL");
        return NULL;
    }

    /* did the user specify the full path to cmd by chance? */
    if (access(cmd, X_OK) == 0) {
        return cmd;
    }

    /* strtok() modifies our original, so we need to make a copy of the
     * PATH environment variable since strtok() will trample it for our
     * process
     */
    if (getenv("PATH") == NULL) {
        syslog(LOG_ERR, "missing PATH in environment table");
        return NULL;
    } else {
        path = strdup(getenv("PATH"));
    }

    /* check each directory in the path with our command */
    dir = strtok(path, ":");
    while (dir != NULL) {
        if (ret != NULL) {
            free(ret);
            ret = NULL;
        }

        if (asprintf(&ret, "%s/%s", dir, cmd) == -1) {
            syslog(LOG_ERR, "memory allocation: %s:%d", __func__, __LINE__);
            return NULL;
        }

        if (access(ret, X_OK) == 0) {
            return ret;
        } else {
            dir = strtok(NULL, ":");
        }
    }

    syslog(LOG_ERR, "%s not found in PATH", cmd);
    return NULL;
}

/* Return PID of specified command.  This function assumes that only one  */
/* process is running by that name.                                       */
pid_t pidof(char *cmd) {
    pid_t ret = -1;
    char *realcmd = NULL, *realproc = NULL, *realwatch = NULL, *procpath = NULL;
    char cmdbuf[PATH_MAX];
    char procbuf[PATH_MAX];
    char watchbuf[PATH_MAX];
    char *ep = NULL;
    DIR *dp = NULL;
    struct dirent *de = NULL;

    if (cmd == NULL) {
        syslog(LOG_ERR, "input to pidof() is NULL");
        return -1;
    }

syslog(LOG_ERR, "%s cmd: |%s|", __func__, cmd);

    if ((realcmd = realpath(which(cmd), cmdbuf)) == NULL) {
        syslog(LOG_ERR, "realpath failure: %s:%d", __func__, __LINE__);
        return -1;
    }

syslog(LOG_ERR, "%s realcmd: |%s|", __func__, realcmd);

    if ((realwatch = realpath(which("vpncwatch"), watchbuf)) == NULL) {
        syslog(LOG_ERR, "realpath failure: %s:%d", __func__, __LINE__);
        return -1;
    }

syslog(LOG_ERR, "%s realwatch: |%s|", __func__, realwatch);

    if ((dp = opendir("/proc")) == NULL) {
        syslog(LOG_ERR, "opendir failure: %s:%d", __func__, __LINE__);
        return -1;
    }

    while ((de = readdir(dp)) != NULL) {
        /* normally, we'd check d_type to see if it's DT_DIR before doing
         * stuff, but uClibc does not set d_type, so we just go anyway if
         * we can perform an strtol() and get something larger than 0.
         */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name(".."))) {
            continue;
        }

syslog(LOG_ERR, "|%s|", de->d_name);
        if (procpath != NULL) {
            free(procpath);
        }

        if (asprintf(&procpath, "/proc/%s/exe", de->d_name) == -1) {
            syslog(LOG_ERR, "memory allocation: %s:%d", __func__, __LINE__);
            return -1;
        }

        if ((realproc = realpath(procpath, procbuf)) == NULL) {
            /* exe not found, which probably means it's a kernel process */
            continue;
        }

        if ((strstr(realproc, realcmd) != NULL) &&
            (strstr(realproc, realwatch) == NULL)) {
            ret = strtol(de->d_name, &ep, 10);

            if (((ret == LONG_MIN || ret == LONG_MAX) && (errno == ERANGE)) ||
                ((ret == 0) && (errno == EINVAL))) {
               syslog(LOG_ERR, "unable to convert %s to int", de->d_name);
               break;
            }

            syslog(LOG_ERR, "found process %s (%d)", de->d_name, ret);
            break;
        }
    }

    if (closedir(dp)) {
        syslog(LOG_ERR, "closedir failure: %s:%d", __func__, __LINE__);
        return -1;
    }

    if (procpath != NULL) {
        free(procpath);
    }

    return ret;
}

/* Check for a /proc entry for the given PID */
int is_running(int pid) {
    char *pidpath = NULL;

    if (asprintf(&pidpath, "/proc/%d/exe", pid) == -1) {
        syslog(LOG_ERR, "memory allocation: %s:%d", __func__, __LINE__);
        return 0;
    }

    if (access(pidpath, R_OK) == -1) {
        return 0;
    } else {
        return 1;
    }
}

/* start the command */
pid_t start_cmd(char *cmd, char *cmdpath, char **argv) {
    int status = 0;
    int cmdpid = 0;
    pid_t pid = 0;

    syslog(LOG_NOTICE, "starting %s", cmdpath);

    if ((pid = fork()) == 0) {
        if (execv(cmdpath, argv) == -1) {
            syslog(LOG_ERR, "execv failure: %s:%d", __func__, __LINE__);
            exit(EXIT_FAILURE);
        }
    } else if (pid == -1) {
        syslog(LOG_ERR, "fork failure: %s:%d", __func__, __LINE__);
        return -1;
    } else {
        if (waitpid(pid, &status, 0) == -1) {
            syslog(LOG_ERR, "waitpid failure: %s:%d", __func__, __LINE__);
            return -1;
        }

        if (WIFSIGNALED(status)) {
            syslog(LOG_ERR, "%s killed with signal %d", cmd, WTERMSIG(status));
            return -1;
        }

        if (!WIFEXITED(status)) {
            syslog(LOG_ERR, "waitpid returned %d", status);
            return -1;
        }

        if ((cmdpid = pidof(cmd)) == -1) {
            syslog(LOG_ERR, "%s is not running", cmd);
        } else {
            syslog(LOG_NOTICE, "%s started", cmd);
        }

        return cmdpid;
    }

    /* never reached */
    return -1;
}

/* stop the command */
void stop_cmd(char *cmd, int cmdpid) {
    int s, i;

    syslog(LOG_NOTICE, "stopping %s", cmd);

    /* try SIGTERM, then SIGKILL */
    for (s = SIGTERM; s <= SIGKILL; s -= (SIGTERM - SIGKILL)) {
        if (!kill(cmdpid, s)) {
            return;
        }

        for (i = 0; i < 30; i++) {
            if (!is_running(cmdpid)) {
                break;
            }

            sleep(1);
        }

        if (!is_running(cmdpid)) {
            return;
        }
    }

    if (is_running(cmdpid)) {
        syslog(LOG_ERR, "%s (%d) didn't die!", cmd, cmdpid);
    }

    return;
}
