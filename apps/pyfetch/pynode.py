#!./pyfetch

import random
import time

from fetchnetwork.swarm import Swarm
from functools import partial

PEERS = [
    "127.0.0.1:9000",
    "127.0.0.1:9001",
    ]

class SwarmAgentNaive(object):
    def __init__(self):
        self.swarm = Swarm(12, 9012, 10012, 3, 100, 1000)

        self.swarm.OnIdle(self.onIdle)
        self.swarm.OnPingFailed(self.onPingFailed)
        self.swarm.OnPingSucceeded(self.onPingSucceeded)
        self.swarm.OnPeerless(self.onPeerless)

        self.swarm.OnNewPeerDiscovered(self.onNewPeerDiscovered)
        self.swarm.OnNewBlockIdFound(self.onNewBlockIdFound)
        self.swarm.OnNewBlockAvailable(self.onNewBlockAvailable)
        self.swarm.OnBlockIdRepeated(self.onBlockIdRepeated)

        self.swarm.Start()

    def onPingFailed(self, host):
        print("Ping failed to:", host)
        self.swarm.AddKarma(host, -5.0);

    def onPingSucceeded(self, host):
        self.swarm.AddKarmaMax(host, 1.0, 3.0);

    def onIdle(self):
        goodPeers = self.swarm.GetPeers(10, -0.5);
        if not goodPeers:
            return
        self.swarm.DoDiscoverBlocks(goodPeers[0], 10);


        weightedPeers = [(x,self.swarm.GetKarma(x)) for x in goodPeers]
        total = sum([ x[1] for x in weightedPeers ])
        weight = random.random() * total

        weightedPeer = weightedPeers[0]
        while weight >= 0 and weightedPeers:
            weightedPeer = weightedPeers.pop(0)
            weight -= weightedPeer[1]

        host = weightedPeer[0]
        self.swarm.DoPing(host);
        self.swarm.DoDiscoverBlocks(host, 10);

    def onPeerless(self):
        for x in PEERS:
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
    agent = SwarmAgentNaive()

    while True:
        print("Zzzz...")
        time.sleep(30)

if __name__ == "__main__":
    main()
