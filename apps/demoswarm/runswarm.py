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

# (clear;cd ../../build/;make swarm) && ./runswarm.py --members 51 --binary ../../build/examples/swarm --caching 1 --initialpeers 2 --maxpeers 4  --idlespeed 100 2>&1 | tee log
#
#

class RunSwarmArgs(object):
    def __init__(self):
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument("--nodetype", help="PyfetchNode or ConstellationNode", type=str, default="ConstellationNode")
        self.parser.add_argument("--members", help="number of swarmers", type=int, default=7)
        self.parser.add_argument("--binary", help="swarm node binary", type=str, default="build/apps/pyfetch/pyfetch apps/pyfetch/pychainnode.py")
        self.parser.add_argument("--initialpeers", help="number of seed peers", type=int, default=2)
        self.parser.add_argument("--maxpeers", help="max peers to discover", type=int, default=4)
        self.parser.add_argument("--idlespeed", help="idle cycle time in MS for a node", type=int, default=1000)
        self.parser.add_argument("--logdir", help="where to put individual logfiles", type=str, default="build/swarmlog/")
        self.parser.add_argument("--startindex", help="Index of first launchable node", type=int, default=0)
        self.parser.add_argument("--debugger", help="Name of debugger", type=str, default="")
        self.parser.add_argument("--clean", help="Name of debugger", default=False, action='store_true')
        self.parser.add_argument("--target", help="mining target", type=int, default=16)

        self.data =  self.parser.parse_args()

    def get(self):
        return self.data

class SwarmWatcher(object):
    def __init__(self, args):
        self.binary = args.binary.split(' ')[0]
        pass

    def watch(self):
        p = subprocess.Popen(
            "ps -ef | grep {} | grep -v -- \"--binary\" | grep -v \"bin/sh\" | grep -v python | grep -v grep | wc -l""".format(self.binary),
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

        peers = set()
        while len(peers)<args.initialpeers:
            rnd = (index + random.randint(0, args.members)) % args.members
            if rnd == index:
                continue
            peers.add(rnd * 20 + PORT_BASE + 1)

        os.makedirs(args.logdir, exist_ok=True)
        os.makedirs("data-{}/".format(self.index), exist_ok=True)

        self.peers = [
            "127.0.0.1:{}".format(x) for x in peers
        ]

        frontargs = re.split(r'\s+', args.binary)

        self.moreargs = frontargs[1:]
        self.frontargs = frontargs[0]

        self.backargs = {
            "-port": "{}".format(self.myport),
            "-peers": ",".join(self.peers),
            "-db-prefix": "data-{}/".format(self.index),
        }

        self.debugger = args.debugger

        {
            "": self.launchRun,
            "gdb": self.launchGDB,
            "lldb": self.launchLLDB,
        }[args.debugger]()

    def launchGDB(self):
        pass

    def launchLLDB(self):
        cmdstr = (self.moreargs +
            [ " ".join([ x[0], x[1] ]) for x in self.backargs.items() ])

        cmdstr = " ".join(cmdstr)

        cmdstr = "screen -S 'lldb-{}' -dm lldb ".format(self.index) + self.frontargs + " -s '/tmp/lldb.run.cmd' -- " + cmdstr
        print(cmdstr)
        self.p = subprocess.Popen("{} | tee {}".format(cmdstr, os.path.join(self.logdir, str(self.index))),
            shell=True
        )


    def launchRun(self):

        cmdstr = ([ self.frontargs ] +
            self.moreargs +
            [ " ".join([ x[0], x[1] ]) for x in self.backargs.items() ])

        cmdstr = " ".join(cmdstr)

        cmdstr = "{} >{} 2>&1".format(
                cmdstr
                , os.path.join(self.logdir, str(self.index))
            )

        print(cmdstr)
        self.p = subprocess.Popen(
            cmdstr,
            shell=True
        )

    def close(self):
        self.p.terminate()
        self.p.kill()


class PyfetchNode(object):
    def __init__(self, index, args, chainident):
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

        os.makedirs(args.logdir, exist_ok=True)

        frontargs = re.split(r'\s+', args.binary)

        self.moreargs = frontargs[1:]
        self.frontargs = frontargs[0]

        self.backargs = {
            "-id": "{}".format(self.index),
            "-maxpeers": "{}".format(self.maxpeers),
            "-target": "{}".format(args.target),
            "-port": "{}".format(PORT_BASE + self.index),
            "-chainident": "{}".format(chainident),
            "-peers": ",".join(self.peers),
            "-idlespeed": "{}".format(args.idlespeed),
        }

        self.debugger = args.debugger

        {
            "": self.launchRun,
            "gdb": self.launchGDB,
            "lldb": self.launchLLDB,
        }[args.debugger]()


    def launchLLDB(self):
        cmdstr = (self.moreargs +
            [ " ".join([ x[0], x[1] ]) for x in self.backargs.items() ])

        cmdstr = " ".join(cmdstr)

        cmdstr = "screen -S 'lldb-{}' -dm lldb ".format(self.index) + self.frontargs + " -s '/tmp/lldb.run.cmd' -- " + cmdstr
        print(cmdstr)

        if logdir:
            cmdstr += " | tee {}".format(os.path.join(self.logdir, str(self.index)))
        else:
            cmdstr += " >/dev/null"

        self.p = subprocess.Popen("{}".format(cmdstr)),
            shell=True
        )

    def launchGDB(self):
        pass

    def killGDB(self):
        pass

    def killRun(self):
        pass

    def killLLDB(self):
        pass

    def launchRun(self):

        cmdstr = ([ self.frontargs ] +
            self.moreargs +
            [ " ".join([ x[0], x[1] ]) for x in self.backargs.items() ])

        cmdstr = " ".join(cmdstr)

        cmdstr = "{} >{}".format(
                cmdstr
                , os.path.join(self.logdir, str(self.index))
            )

        print(cmdstr)
        self.p = subprocess.Popen(
            cmdstr,
            shell=True
        )

    def close(self):
        {
            "": self.killRun,
            "gdb": self.killGDB,
            "lldb": self.killLLDB,
        }[self.debugger]()

        self.p.terminate()
        self.p.kill()


class Swarm(object):
    def __init__(self, args):
        chainident = int(time.time())
        builder = {
            "ConstellationNode": ConstellationNode,
            "PyfetchNode": PyfetchNode,
        }[args.nodetype]
        self.nodes = dict([ (x, builder(x, args, chainident)) for x in range(0, args.members)])

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


def main():
    swarmArgs = RunSwarmArgs()
    args = swarmArgs.get()

    with open("/tmp/lldb.run.cmd", "w") as fn:
        fn.write("settings set thread-format thread #${thread.index}: tid = ${thread.id}{, name = ${thread.name}}{, function: ${function.name}} {, stop reason = ${thread.stop-reason}}{, return = ${thread.return-value}}\\n\n")
        fn.write("breakpoint set --name abort\n")
        fn.write("breakpoint set --name exit\n")
        fn.write("run\n");

    if args.clean:
        killall()

    with createSwarm(args) as swarm:
        with createSwarmWatcher(args) as watcher:
            while watcher.watch():
                time.sleep(2)


if __name__ == "__main__":
    main()
