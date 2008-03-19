#!/usr/bin/env python

## Copyright (C) 2006 Red Hat, Inc.
## Written by Gary Benson <gbenson@redhat.com>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

import os
import signal
import sys
import syslog
import time
import traceback

class Error(Exception):
    pass

def which(cmd):
    for path in os.environ["PATH"].split(":"):
        path = os.path.join(path, cmd)
        if os.access(path, os.X_OK):
            return path
    raise Error, "no %s in (%s)" % (cmd, os.environ["PATH"])

def pidof(cmd):
    if not os.path.dirname(cmd):
        cmd = which(cmd)
    cmd = os.path.realpath(cmd)
    pids = []
    seen = False
    for pid in os.listdir("/proc"):
        try:
            pid = int(pid)
        except ValueError:
            continue
        seen = True
        path = os.path.join("/proc", str(pid), "exe")
        try:
            if os.path.realpath(path) == cmd:
                pids.append(pid)
        except OSError:
            pass
    assert seen
    return pids

class Watcher:
    def __init__(self, cmd, args):
        self.name = os.path.basename(cmd)
        if not os.path.dirname(cmd):
            cmd = which(cmd)
        self.cmd = os.path.realpath(cmd)
        self.args = args

    def start(self):
        syslog.syslog(syslog.LOG_NOTICE, "starting %s" % self.name)
        pid = os.fork()
        if pid == 0:
            os.execv(self.cmd, [self.name] + self.args)
        status = os.waitpid(pid, 0)[1]
        if os.WIFSIGNALED(status):
            raise Error, "%s killed with signal %d" % (
                self.cmd, os.WTERMSIG(status))
        if not os.WIFEXITED(status):
            raise RuntimeError, "os.waitpid returned %d" % status
        status = os.WEXITSTATUS(status)
        if status:
            raise Error, "%s exited with code %d" % (self.cmd, status)
        pids = pidof(self.cmd)
        if not pids:
            raise Error, "%s is not running!" % self.cmd
        if len(pids) != 1:
            pids = ", ".join(map(str, pids))
            raise Error, "multiple copies of %s running!" % (self.cmd, pids)
        self.pid = pids[0]
        syslog.syslog(syslog.LOG_NOTICE, "%s started" % self.name)

    def detach(self):
        if os.fork():
            sys.exit()
        os.setsid()
        if os.fork():
            sys.exit()
        null = os.open("/dev/null", os.O_RDWR)
        for fd in range(3):
            os.dup2(null, fd)

    def isrunning(self):
        return os.access(os.path.join("/proc", str(self.pid)), os.F_OK)

    def stop(self):
         syslog.syslog(syslog.LOG_NOTICE, "stopping %s" % self.name)
         for sig in (signal.SIGTERM, signal.SIGKILL):
             try:
                 os.kill(self.pid, signal.SIGTERM)
             except OSError:
                 continue
             for i in xrange(30):
                 if not self.isrunning():
                     break
                 time.sleep(1)
             if not self.isrunning():
                 break
         if self.isrunning():
             raise Error, "%s (%d) didn't die!" % (self.name, self.pid)

    def signal(self, num, frame):
        if num == signal.SIGHUP:
            self.do_restart = True
        if num == signal.SIGTERM:
            self.do_exit = True

    def run(self):
        syslog.openlog("vpnc-watch", syslog.LOG_PID, syslog.LOG_DAEMON)
        pids = pidof(self.cmd)
        if pids:
            pids = ", ".join(map(str, pids))
            raise Error, "%s already running (%s)" % (self.cmd, pids)
        self.start()
        self.detach()
        try:
            signal.signal(signal.SIGHUP, self.signal)
            signal.signal(signal.SIGTERM, self.signal)
            
            self.do_exit = False
            while not self.do_exit:
                self.do_restart = False
                time.sleep(1)

                running = self.isrunning()
                if not running:
                    syslog.syslog(syslog.LOG_WARNING, "%s died" % self.name)
                elif self.do_exit or self.do_restart:
                    self.stop()
                if self.do_restart or not running:
                    self.start()
            syslog.syslog(syslog.LOG_INFO, "exiting")

        except Error, e:
            syslog.syslog(syslog.LOG_ERR, "error: " + str(e))
            sys.exit(1)
        except:
            msg = "".join(apply(traceback.format_exception, sys.exc_info()))
            for line in msg.split("\n"):
                if line:
                    syslog.syslog(syslog.LOG_ERR, line)
            sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print >>sys.stderr, "usage: %s COMMAND [ARGS]" % sys.argv[0]
        sys.exit(1)
    try:
        Watcher(sys.argv[1], sys.argv[2:]).run()
    except Error, e:
        print >>sys.stderr, "error:", e
        sys.exit(1)
