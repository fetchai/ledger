#!/usr/bin/env python3

import sys
import os
import re
import argparse
from contextlib import contextmanager
import subprocess
import time
import random
import functools

PORT_BASE = 9000

# (clear;cd ../../build/;make swarm) && ./runswarm.py --members 51 --binary ../../build/examples/swarm --caching 1 --initialpeers 2 --maxpeers 4  --idlespeed 100 2>&1 | tee log
#


# ['/Users/katie/Coding/fetch-ledger7/build/apps/constellation/constellation', '-port', '8060',
# '-block-interval', '5000', '-transient-peers', '2', '-max-peers', '4',
# '-peers-update-cycle-ms', '200', '-block-interval', '3000',
# '-peers', 'tcp://127.0.0.1:8041,tcp://127.0.0.1:8021']

class RunSwarmArgs(object):
    def __init__(self):
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument(
            "--nodetype",
            help="PyfetchNode or ConstellationNode",
            type=str,
            default="ConstellationNode")
        self.parser.add_argument(
            "--members", help="number of swarmers", type=int, default=7)
        self.parser.add_argument(
            "--binary", help="swarm node binary", type=str,
            default="build/apps/pyfetch/pyfetch apps/pyfetch/pychainnode.py")
        self.parser.add_argument(
            "--initialpeers", help="number of seed peers", type=int, default=2)
        self.parser.add_argument(
            "--maxpeers", help="max peers to discover", type=int, default=4)
        self.parser.add_argument(
            "--idlespeed",
            help="idle cycle time in MS for a node",
            type=int,
            default=1000)
        self.parser.add_argument(
            "--logdir",
            help="where to put individual logfiles",
            type=str,
            default="../../build/swarmlog/")
        self.parser.add_argument(
            "--startindex",
            help="Index of first launchable node",
            type=int,
            default=0)
        self.parser.add_argument(
            "--debugger", help="Name of debugger", type=str, default="")
        self.parser.add_argument(
            "--clean",
            help="Name of debugger",
            default=False,
            action='store_true')
        self.parser.add_argument(
            "--target", help="mining target", type=int, default=16)
        self.parser.add_argument(
            "--lo", help="start at this member number", type=int, default=0)
        self.parser.add_argument(
            "--debugfirst",
            help="run the first N in the debugger",
            type=int,
            default=0)

        self.data = self.parser.parse_args()

    def get(self):
        return self.data


class SwarmWatcher(object):
    def __init__(self, args):
        self.binary = args.binary.split(' ')[0]
        pass

    def watch(self):
        p = subprocess.Popen(
            "ps -ef | grep {} | grep -v -- \"--binary\" | grep -v \"bin/sh\" | grep -v python | grep -v grep | wc -l"
            "".format(
                self.binary),
            shell=True,
            stdout=subprocess.PIPE)
        out, err = p.communicate()
        alive = out.decode("utf-8").split('\n')
        alive = int(alive[0])
        print("Still Alive: ", alive)
        if not alive:
            return False
        return True

    def close(self):
        pass


def killall():
    cmd = "clear; ps -ef | grep pyfetch/pyfetch | grep -v 'bin/sh' | cut -c8-1000 | cut -d' ' -f1 | xargs kill"
    p = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE
                         )
    out, err = p.communicate()


class ConstellationNode(object):
    def __init__(self, index, args, chainident):
        self.peercount = args.initialpeers
        self.maxpeers = args.maxpeers
        self.myport = PORT_BASE + index * 20
        self.logdir = args.logdir
        self.index = index
        self.debugger = args.debugger

        peers = set()
        while len(peers) < args.initialpeers:
            rnd = (index + random.randint(0, args.members)) % args.members
            if rnd == index:
                continue
            peers.add(rnd * 20 + PORT_BASE + 1)

        os.makedirs(args.logdir, exist_ok=True)

        self.peers = [
            "tcp://127.0.0.1:{}".format(x) for x in peers
        ]

        frontargs = re.split(r'\s+', args.binary)

        arg_binary_name = frontargs[0]
        self.dir = os.path.join(frontargs[1], "data-{}/".format(self.index))

        os.makedirs(self.dir, exist_ok=True)

        self.moreargs = []
        self.frontargs = frontargs[0]

        if index == 0:
            self.moreargs.append('-mine')

        self.backargs = {
            '-block-interval': '5000',
            '-transient-peers': '2',
            '-max-peers': '4',
            '-peers-update-cycle-ms': '200',
            "-port": "{}".format(self.myport),
            "-peers": ",".join(self.peers)
        }

    def Run(self):
        {
            "": self.launchRun,
            "gdb": self.launchGDB,
            "lldb": self.launchLLDB,
        }[self.debugger]()

    def launchGDB(self):
        pass

    def launchLLDB(self):
        cmdstr = (self.moreargs +
                  [" ".join([x[0], x[1]]) for x in self.backargs.items()])

        cmdstr = " ".join(cmdstr)

        cmdstr = "screen -S 'lldb-{}' -dm lldb ".format(
            self.index) + self.frontargs + " -s '/tmp/lldb.run.cmd' -- " + cmdstr
        print("CMD=", cmdstr)
        print("CWD=", self.dir)
        self.p = subprocess.Popen("{}".format(cmdstr),
                                  shell=True,
                                  cwd=self.dir)

    def launchRun(self):
        cmdstr = ([self.frontargs] +
                  self.moreargs +
                  [" ".join([x[0], x[1]]) for x in self.backargs.items()])

        cmdstr = " ".join(cmdstr)

        cmdstr = "{} >{} 2>&1".format(
            cmdstr, os.path.join(self.logdir, str(self.index))
        )

        print(cmdstr)
        self.p = subprocess.Popen(
            cmdstr,
            shell=True,
            cwd=self.dir,
            stdout=self._logfile,
            stderr=subprocess.STDOUT)

    def close(self):
        self.p.terminate()
        self.p.kill()


class Swarm(object):
    def __init__(self, args):
        chainident = int(time.time())
        builder = ConstellationNode
        self.nodes = dict([(x, builder(x, args, chainident))
                           for x in range(args.lo, args.members)])

    def Run(self):
        for node in self.nodes.values():
            node.Run()

    def VisitNodes(self, visitor):
        for node in self.nodes.values():
            visitor(node)

    def close(self):
        for node in self.nodes.values():
            node.close()


def ClearDebuggerFromNode(args, node):
    if node.index >= args.debugfirst:
        node.debugger = ""


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


def main():
    swarmArgs = RunSwarmArgs()
    args = swarmArgs.get()

    if args.debugger:
        if not args.debugfirst:
            args.debugfirst = args.members

    with open("/tmp/lldb.run.cmd", "w") as fn:
        #fn.write("br s -M bad_weak_ptr\n")
        #fn.write("br s -M system_error\n")
        #fn.write("br s -n exit\n")
        #fn.write("br s -n abort\n")
        #fn.write("br s -M TCPServer::~TCPServer()\n")
        fn.write("run\n")

    if args.clean:
        killall()

    with createSwarm(args) as swarm:
        swarm.VisitNodes(functools.partial(ClearDebuggerFromNode, args))
        swarm.Run()
        with createSwarmWatcher(args) as watcher:
            while watcher.watch():
                time.sleep(2)


if __name__ == "__main__":
    main()
