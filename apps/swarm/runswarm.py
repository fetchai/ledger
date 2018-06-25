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
        self.parser.add_argument("--caching", help="keep connections cached", type=bool)
        self.parser.add_argument("--solvespeed", help="chance of solving a block as 1/N", type=int, default=1000)
        self.parser.add_argument("--idlespeed", help="idle cycle time in MS for a node", type=int, default=1000)

        self.data =  self.parser.parse_args()
        print (self.data)

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
    

class Node(object):
    def __init__(self, index, args):
        peercount = args.initialpeers
        maxpeers = args.maxpeers
        myport = PORT_BASE + index

        peers = set()
        while len(peers)<args.initialpeers:
            rnd = (index + random.randint(0, args.members)) % args.members
            if rnd == index:
                continue
            peers.add(rnd + PORT_BASE)

        peers = [
            "127.0.0.1:{}".format(x) for x in peers
        ]
        self.myargs = [
            args.binary,
            "-id", "{}".format(index),
            "-maxpeers", "{}".format(maxpeers),
            "-port", "{}".format(PORT_BASE + index),
            "-peers", ",".join(peers),
            "-caching", "1" if args.caching else "0"
        ]
        print(" ".join(self.myargs))
        #self.p = subprocess.Popen(self.myargs)
        self.p = subprocess.Popen("{} 2>&1 |tee swarmlog/{}".format(
                " ".join(self.myargs),
                index
            ),
            shell=True
        )

    def close(self):
        self.p.terminate()
        self.p.kill()


def main():
    swarmArgs = RunSwarmArgs()
    args = swarmArgs.get()
    with createSwarm(args) as swarm:
        time.sleep(2000)


if __name__ == "__main__":
    main()
