#!./pyfetch

import argparse
import random
import sys
import time

from fetchnetwork.swarm import Swarm, say
from fetchledger.chain import MainChain, MainChainBlock
from functools import partial

#from fetchnetwork.swarm import ostream_redirect

PEERS = [
    "127.0.0.1:9000",
    "127.0.0.1:9001",
    ]

class SwarmAgentNaive(object):
    def __init__(self, idnum, rpcPort, httpPort, maxpeers, idlespeed, peers, target):
        self.swarm = Swarm(idnum, rpcPort, httpPort, maxpeers, idlespeed, target)
        self.idnum = idnum

        self.peerlist = peers.split(",")
        self.blockCounter = 0

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

        self.in_progress = set()

        say(self.mainchain.totalBlocks())
        self.swarm.Start()


    def onPingFailed(self, host):
        say("PYCHAINNODE===> Ping failed to:", host)
        self.swarm.AddKarma(host, -5.0);
        self.in_progress.discard(host)

    def onPingSucceeded(self, host):
        self.swarm.AddKarmaMax(host, 10.0, 30.0);
        self.in_progress.discard(host)

    def onIdle(self):
        say("idle")
        goodPeers = self.swarm.GetPeers(10, -0.5)

        goodPeers = [ x for x in goodPeers if x not in self.in_progress ]

        if not goodPeers:
            say("quiet")
            return 100

        hosts = []

        weightedPeers = [(x,self.swarm.GetKarma(x)) for x in goodPeers]
        total = sum([ x[1] for x in weightedPeers ])
        weight = random.random() * total
        weightedPeer = weightedPeers[0]
        while weight >= 0 and weightedPeers:
            weightedPeer = weightedPeers.pop(0)
            weight -= weightedPeer[1]
        hosts.append(weightedPeer[0])
        hosts.append(goodPeers[int(random.random() * len(goodPeers))])

        for host in hosts:
            if host not in self.in_progress:
                self.in_progress.add(host)
                self.swarm.DoPing(host);
                self.swarm.DoDiscoverBlocks(host, 10);
            else:
                say("PYCHAINNODE===> Ping deferred to:", host)

        return 0

    def onPeerless(self):
        for x in self.peerlist:
            self.swarm.DoPing(x)

    def onNewPeerDiscovered(self, host):
        if host == self.swarm.queryOwnLocation():
            return
        say("PYCHAINNODE===> NEW PEER", host);

    def onNewBlockIdFound(self, host, blockid):
        say("PYCHAINNODE===> WOW - ", blockid)
        self.swarm.AddKarmaMax(host, 2.0, 30.0);

    def onBlockIdRepeated(self, host, blockid):
        # Awwww, we know about this.
        self.swarm.AddKarmaMax(host, 1.0, 10.0);
        pass

    def onLooseBlock(self, host, blockid):
        say("PYCHAINNODE===> LOOSE: ", host, blockid)
        self.swarm.DoGetBlock(host, blockid)

    def onBlockSupplied(self, host, blockid):
        say("PYCHAINNODE===> DELIVERED: ", host, blockid)

    def onBlockNotSupplied(self, host, blockid):
        say("PYCHAINNODE===> NOT DELIVERED: ", host, blockid)


def run(config):
    agent = SwarmAgentNaive(
        config.id,
        config.port,
        config.port + 1000,
        config.maxpeers,
        config.idlespeed,
        config.peers,
        config.target
    )

    while True:
        time.sleep(2)

def main():
    params = argparse.ArgumentParser(description='I am a Fetch node.')

    params.add_argument("-target",         type=int, help="Mining target.", default=16);
    params.add_argument("-id",             type=int, help="Identifier number for this node.", default=1);
    params.add_argument("-port",           type=int, help="Which port to run on.", default=9012);
    params.add_argument("-maxpeers",       type=int, help="Ideally how many peers to maintain good connections to.", default=3);
    params.add_argument("-idlespeed",      type=int, help="The rate, in milliseconds, of generating idle events to the Swarm Agent.", default=100);
    params.add_argument("-peers",          type=str, help="Comma separated list of peer locations.", default=",".join(PEERS));

    config = params.parse_args(sys.argv[1:])

    run(config)

if __name__ == "__main__":
    main()
