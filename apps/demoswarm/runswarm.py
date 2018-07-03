#!/usr/bin/env python3

import sys
import os
import re
import argparse
from contextlib import contextmanager
import subprocess
import time
import random

PORT_BASE = 9000

# (clear;cd ../../build/;make swarm) && ./runswarm.py --members 51 --binary ../../build/examples/swarm --caching 1 --initialpeers 2 --maxpeers 4  --idlespeed 100 --solvespeed 10000 2>&1 | tee log
#
#

class RunSwarmArgs(object):
    def __init__(self):
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument("--members", help="number of swarmers", type=int)
        self.parser.add_argument("--binary", help="swarm node binary", type=str)
        self.parser.add_argument("--initialpeers", help="number of seed peers", type=int)
        self.parser.add_argument("--maxpeers", help="max peers to discover", type=int)
        self.parser.add_argument("--solvespeed", help="chance of solving a block as 1/N", type=int, default=1000)
        self.parser.add_argument("--idlespeed", help="idle cycle time in MS for a node", type=int, default=1000)
        self.parser.add_argument("--logdir", help="where to put individual logfiles", type=str, default="./")
        self.parser.add_argument("--startindex", help="Index of first launchable node", type=int, default=0)
        self.parser.add_argument("--debugger", help="Name of debugger", type=str, default="")
        self.parser.add_argument("--clean", help="Name of debugger", default=False, action='store_true')

        self.data =  self.parser.parse_args()

    def get(self):
        return self.data

class Swarm(object):
    def __init__(self, args):
        self.nodes = dict([ (x, Node(x, args)) for x in range(0, args.members)])

    def close(self):
        for node in self.nodes.values():
            node.close()


@contextmanager
def createSwarm(args):
    swarm = Swarm(args)
    yield swarm
    swarm.close()

@contextmanager
def createSwarmWatcher(*args, **kwargs):
    x = SwarmWatcher(*args, **kwargs)
    yield x
    x.close()


class SwarmWatcher(object):
    def __init__(self):
        pass

    def watch(self):
        p = subprocess.Popen(
            "ps -ef | grep apps/pyfetch/pyfetch | grep -- \"-id\" | grep -v bin/sh | grep -v python | grep -v grep | wc -l",
            shell=True,
            stdout=subprocess.PIPE
            )
        out, err = p.communicate()
        alive = out.decode("utf-8").split('\n')
        alive = int(alive[0])
        print("Still Alive: ", alive)
        if not alive:
            return False
        return True

def killall():
    cmd = "clear; ps -ef | grep pyfetch/pyfetch | grep -v 'bin/sh' | cut -c8-1000 | cut -d' ' -f1 | xargs kill"
    p = subprocess.Popen(cmd,
        shell=True,
        stdout=subprocess.PIPE
    )
    out, err = p.communicate()

class Node(object):
    def __init__(self, index, args):
        self.peercount = args.initialpeers
        self.maxpeers = args.maxpeers
        self.myport = PORT_BASE + index
        self.logdir = args.logdir
        self.index = index

        peers = set()
        while len(peers)<args.initialpeers:
            rnd = (index + random.randint(0, args.members)) % args.members
            if rnd == index:
                continue
            peers.add(rnd + PORT_BASE)

        self.peers = [
            "127.0.0.1:{}".format(x) for x in peers
        ]

        frontargs = re.split(r'\s+', args.binary)

        self.moreargs = frontargs[1:]
        self.frontargs = frontargs[0]

        self.backargs = {
            "-id": "{}".format(self.index),
            "-maxpeers": "{}".format(self.maxpeers),
            "-port": "{}".format(PORT_BASE + self.index),
            "-peers": ",".join(self.peers),
            "-solvespeed": "{}".format(args.solvespeed),
            "-idlespeed": "{}".format(args.idlespeed),
        }

        {
            "": self.launchRun,
            "gdb": self.launchGDB,
            "lldb": self.launchLLDB,
        }[args.debugger]()


    def launchLLDB(self):
        pass

    def launchGDB(self):
        pass

    def launchRun(self):

        cmdstr = ([ self.frontargs ] +
            self.moreargs +
            [ " ".join([ x[0], x[1] ]) for x in self.backargs.items() ])

        cmdstr = " ".join(cmdstr)
        print(cmdstr)
        self.p = subprocess.Popen("{} >{}".format(cmdstr, os.path.join(self.logdir, str(self.index))),
            shell=True
        )

    def close(self):
        self.p.terminate()
        self.p.kill()


def main():
    swarmArgs = RunSwarmArgs()
    args = swarmArgs.get()

    if args.clean:
        killall()

    with createSwarm(args) as swarm:
        with createSwarmWatcher() as watcher:
            while watcher.watch():
                time.sleep(2)


if __name__ == "__main__":
    main()
