#!/usr/bin/env python3

import time
import re

def follow(thefile):
    thefile.seek(0,2)
    while True:
        line = thefile.readline()
        if not line:
            time.sleep(0.1)
            continue
        yield line

FINDER=re.compile(r"\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d.\d\d\d.{0,10}#([^:]+):")

class WatchedLogfile:
    def __init__(self, filename):
        self.filename = filename
        self.fh = open(filename, "r")
        self.byThread = {}

    def lineToThreadId(self, line):
        m = re.search(FINDER, line)
        if m:
            return m.groups(1)
        return None

    def poll(self):
        while True:
            line = self.fh.readline()

            if not line:
                return

            threadid = self.lineToThreadId(line)
            line = line.rstrip()

            if threadid != None:
                self.byThread.setdefault(threadid, []).append(line)
                if 'Connection timed out' in line:
                    print("{}:{}".format(self.filename, self.byThread[threadid][-2]))
                    print("{}:{}".format(self.filename, self.byThread[threadid][-1]))


def main():
    watchers = [ WatchedLogfile("build/swarmlog/"+str(x)) for x in range(0, 7) ]
    while True:
        for watcher in watchers:
            watcher.poll()
        time.sleep(0.1)


if __name__ == "__main__":
    main()
