#!./pyfetch

import argparse
import random
import sys
import time
import datetime

from fetchnetwork.swarm import Swarm, say
from fetchledger.chain import MainChain, MainChainBlock
from functools import partial

#from fetchnetwork.swarm import ostream_redirect

PEERS = [
    "127.0.0.1:9000",
    "127.0.0.1:9001",
    ]

class SwarmAgentNaive(object):
    def __init__(self, idnum, rpcPort, httpPort,
                     maxpeers, idlespeed, peers, target, chainident, introductions):
        self.swarm = Swarm(idnum, rpcPort, httpPort,
                               maxpeers, idlespeed, target, chainident)
        self.idnum = idnum

        self.introductions = introductions

        self.peerlist = peers.split(",")
        self.blockCounter = 0
        self.maxpeers = maxpeers

        self.rootblock = MainChainBlock()
        self.mainchain = MainChain(self.rootblock)

        say("SETTING ON IDLE PYCHAIN")
        self.swarm.OnIdle(self.onIdle)
        say("SET ON IDLE PYCHAIN")

        self.swarm.OnPingFailed(self.onPingFailed)
        self.swarm.OnPingSucceeded(self.onPingSucceeded)
        self.swarm.OnPeerless(self.onPeerless)

        self.swarm.OnNewPeerDiscovered(self.onNewPeerDiscovered)
        self.swarm.OnNewBlockIdFound(self.onNewBlockIdFound)
        self.swarm.OnBlockIdRepeated(self.onBlockIdRepeated)
        self.swarm.OnLooseBlock(self.onLooseBlock)
        self.swarm.OnBlockSupplied(self.onBlockSupplied)
        self.swarm.OnBlockNotSupplied(self.onBlockNotSupplied)
        self.timeOfLastRemoteNewBlock = datetime.datetime.now()

        say(self.mainchain.totalBlocks())
        self.swarm.Start()

        self.inflight = set()
        self.subs = []

    def onPingFailed(self, host):
        say("PYCHAINNODE===> Ping failed to:", host)
        self.swarm.AddKarma(host, -5.0);
        self.inflight.discard(host)

    def onPingSucceeded(self, host):
        self.swarm.AddKarmaMax(host, 3.0, 15.0);
        self.inflight.discard(host)

    def RequestChain(self, host):
        if (datetime.datetime.now() - self.timeOfLastRemoteNewBlock).total_seconds() > 10:
            self.inflight = set()
        if host in self.inflight:
            return
        self.inflight.add(host)
        self.swarm.DoLoadChain(host, 30)
        self.swarm.DoPing(host)

    def SendPing(self, host):
        if (datetime.datetime.now() - self.timeOfLastRemoteNewBlock).total_seconds() > 10:
            inflight = set()
        if host in self.inflight:
            return
        self.inflight.add(host)
        self.swarm.DoPing(host)

    def CreateSubscription(self, host):
        if host in self.subs:
            #say("AGENT_API PYCHAIN IGNORING ", host);
            self.SendPing(host)
            return

        while len(self.subs) >= self.maxpeers:
            x = self.subs.pop(0)
            say("AGENT_API PYCHAIN SUBSCRIBE ", x);
            self.swarm.DoStopBlockDiscover(x, 0)
        self.subs.append(host)
        self.swarm.DoDiscoverBlocks(host, 0);
        say("AGENT_API PYCHAIN SUBSCRIBE ", host);
        self.subs.append(host)


    def onIdle(self):

        mode = ""

        goodPeers = self.swarm.GetPeers(10, -0.5)

        goodPeers = [ goodPeerFilter for goodPeerFilter in goodPeers if goodPeerFilter not in self.introductions ]

        if not goodPeers:
            say("quiet")
            return 100

        weightedPeers = [(x,self.swarm.GetKarma(x)) for x in goodPeers]
        total = sum([ x[1] for x in weightedPeers ])
        weight = random.random() * total

        weightedPeer = weightedPeers[0]
        while weight >= 0 and weightedPeers:
            weightedPeer = weightedPeers.pop(0)
            weight -= weightedPeer[1]

        host = weightedPeer[0]

        if mode == "chain":
            self.RequestChain(host)
        else:
            self.CreateSubscription(host)

        return 0

    def onPeerless(self):
        for peerListMember in self.peerlist:
            self.swarm.DoPing(peerListMember)
        for introListMember in self.introductions:
            self.swarm.AddKarmaMax(introListMember, 100.0, 100.0);

    def onNewPeerDiscovered(self, host):
        if host == self.swarm.queryOwnLocation():
            return
        #say("AGENT_API PYCHAIN NEW PEER ", host);

    def onNewBlockIdFound(self, host, blockid):
        say("AGENT_API PYCHAIN NEWBLOCK ", blockid)
        self.swarm.AddKarmaMax(host, 2.0, 3.0);
        #self.timeOfLastRemoteNewBlock = datetime.datetime.now()

    def onBlockIdRepeated(self, host, blockid):
        # Awwww, we know about this.
        self.swarm.AddKarmaMax(host, 1.0, 1.0);

    def onLooseBlock(self, host, blockid):
        say("AGENT_API PYCHAIN LOOSE ", host, ' ', blockid)
        self.swarm.DoGetBlock(host, blockid)

    def onBlockSupplied(self, host, blockid):
        say("AGENT_API PYCHAIN DELIVERED ", host, ' ', blockid)
        self.swarm.AddKarmaMax(host, 3.0, 15.0);

    def onBlockNotSupplied(self, host, blockid):
        say("AGENT_API PYCHAIN FAILED  ", host, ' ', blockid)
        self.swarm.AddKarma(host, -2.0)


def run(config):
    agent = SwarmAgentNaive(
        config.id,
        config.port,
        config.port + 1000,
        config.maxpeers,
        config.idlespeed,
        config.peers,
        config.target,
        config.chainident,
        [ "127.0.0.1:" + str(config.port) ] if config.introduce else []
    )

    while True:
        time.sleep(2)

def main():
    params = argparse.ArgumentParser(description='I am a Fetch node.')

    params.add_argument("-target",         type=int, help="Mining target.", default=16)
    params.add_argument("-id",             type=int, help="Identifier number for this node.", default=1)
    params.add_argument("-port",           type=int, help="Which port to run on.", default=9012)
    params.add_argument("-maxpeers",       type=int, help="Ideally how many peers to maintain good connections to.", default=3)
    params.add_argument("-idlespeed",      type=int, help="The rate, in milliseconds, of generating idle events to the Swarm Agent.", default=100)
    params.add_argument("-peers",          type=str, help="Comma separated list of peer locations.", default=",".join(PEERS))
    params.add_argument("-introduce",      default=False, help="Promote myself as a peer in order to join swarm.", action='store_true')
    params.add_argument("-chainident",     help="a number which represents which chain this is part of (for monitoring to use)", type=int, default=0)

    config = params.parse_args(sys.argv[1:])

    run(config)

if __name__ == "__main__":
    main()


DETAILS="""

lldb build/apps/pyfetch/pyfetch -- apps/pyfetch/pychainnode.py -id 10 -maxpeers 4 -target 16 -port 9010 -peers 127.0.0.1:9001,127.0.0.1:9005 -idlespeed 50 --introduce


(cd build;make -j100) && ./apps/demoswarm/runswarm.py --binary "./build/apps/pyfetch/pyfetch ./apps/pyfetch/pychainnode.py" --members 7 --maxpeers 3 --initialpeers 2 --idlespeed 50 --target 18

"""
