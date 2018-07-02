#!./pyfetch

import argparse
import random
import sys
import time

from fetchnetwork.swarm import Swarm
from fetchledger.chain import MainChain, MainChainBlock
from functools import partial

from fetchnetwork.swarm import Swarm

PEERS = [
    "127.0.0.1:9000",
    "127.0.0.1:9001",
    ]

class SwarmAgentNaive(object):
    def __init__(self, idnum, rpcPort, httpPort, maxpeers, idlespeed, solvespeed, peers):
        self.swarm = Swarm(idnum, rpcPort, httpPort, maxpeers, idlespeed, solvespeed)
        self.idnum = idnum

        self.peerlist = peers.split(",")
        self.solvespeed = solvespeed

        self.rootblock = MainChainBlock()
        self.mainchain = MainChain(self.rootblock)

        self.swarm.OnIdle(self.onIdle)
        self.swarm.OnPingFailed(self.onPingFailed)
        self.swarm.OnPingSucceeded(self.onPingSucceeded)
        self.swarm.OnPeerless(self.onPeerless)

        self.swarm.OnNewPeerDiscovered(self.onNewPeerDiscovered)
        self.swarm.OnNewBlockIdFound(self.onNewBlockIdFound)
        self.swarm.OnNewBlockAvailable(self.onNewBlockAvailable)
        self.swarm.OnBlockIdRepeated(self.onBlockIdRepeated)

        print(self.mainchain.totalBlocks())
        self.swarm.Start()

    def onPingFailed(self, host):
        print("Ping failed to:", host)
        self.swarm.AddKarma(host, -5.0);

    def onPingSucceeded(self, host):
        self.swarm.AddKarmaMax(host, 1.0, 3.0);

    def onIdle(self):
        goodPeers = self.swarm.GetPeers(10, -0.5)
        if not goodPeers:
            print("OnIdle", 1.1)
            return
        print("OnIdle", 2)

        #self.swarm.DoDiscoverBlocks(goodPeers[0], 10)
        print("OnIdle", 3)

        if random.randint(0, self.solvespeed) == 0:
            self.swarm.DoBlockSolved("hvgfjhfgbshdv")

        weightedPeers = [(x,self.swarm.GetKarma(x)) for x in goodPeers]
        total = sum([ x[1] for x in weightedPeers ])
        weight = random.random() * total

        weightedPeer = weightedPeers[0]
        while weight >= 0 and weightedPeers:
            weightedPeer = weightedPeers.pop(0)
            weight -= weightedPeer[1]

        host = weightedPeer[0]
        self.swarm.DoPing(host);
        #self.swarm.DoDiscoverBlocks(host, 10);

    def onPeerless(self):
        for x in self.peerlist:
            self.swarm.DoPing(x)

    def onNewPeerDiscovered(self, host):
        if host == self.swarm.queryOwnLocation():
            return
        print("NEW PEER", host);
        self.swarm.DoPing(host)

    def onNewBlockIdFound(self, host, blockid):
        print("WOW - ", blockid)
        self.swarm.AddKarmaMax(host, 1.0, 6.0);
        self.swarm.DoGetBlock(host, blockid);

    def onBlockIdRepeated(self, host, blockid):
        # Awwww, we know about this.
        pass

    def onNewBlockAvailable(self, host, blockid):
        print("GOT - ", blockid)
        print(self.swarm.GetBlock(blockid))

def main():
    params = argparse.ArgumentParser(description='I am a Fetch node.')

    params.add_argument("-id",             type=int, help="Identifier number for this node.", default=1);
    params.add_argument("-port",           type=int, help="Which port to run on.", default=9012);
    params.add_argument("-maxpeers",       type=int, help="Ideally how many peers to maintain good connections to.", default=3);
    params.add_argument("-solvespeed",     type=int, help="The rate of generating block solutions.", default=1000);
    params.add_argument("-idlespeed",      type=int, help="The rate, in milliseconds, of generating idle events to the Swarm Agent.", default=100);
    params.add_argument("-peers",          type=str, help="Comma separated list of peer locations.", default=",".join(PEERS));

    config = params.parse_args(sys.argv[1:])

    agent = SwarmAgentNaive(
        config.id,
        config.port,
        config.port + 1000,
        config.maxpeers,
        config.idlespeed,
        config.solvespeed,
        config.peers
        )

    while True:
        print("Zzzz...")
        time.sleep(30)

if __name__ == "__main__":
    main()
