#!./pyfetch

import argparse
import random
import sys
import time
import datetime
import json

import trueskill

from fetchnetwork.swarm import Swarm, say
from fetchledger.chain import MainChain, MainChainBlock

from functools import partial
#from fetchnetwork.swarm import ostream_redirect
# FIXME: neither functools.partial nor ostream_redirect are used. Remove?

PEERS = [
    "127.0.0.1:9000",
    "127.0.0.1:9001",
    ]


class SwarmAgentBayesian(object):
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

        self.ts = trueskill.TrueSkill(
            mu=100, sigma=100/6, beta=100/12, tau=1/6, draw_probability=0.2)
        self.prior = self.ts.Rating(mu=100)
        self.believe = {}
        self.reference = {
            'new_data': self.ts.Rating(mu=100, sigma=100/6),
            'already_known': self.ts.Rating(mu=80, sigma=100/12),
            'connection_problem': self.ts.Rating(mu=50, sigma=100/2),
            'bad_block': self.ts.Rating(mu=0, sigma=100/24),
        }

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

    def update_believe(self, host, action):
        """Update honesty believe."""
        if host not in self.believe:
            self.believe[host] = self.prior

        current = self.believe[host]
        rate = self.ts.rate_1vs1

        if action == 'ping_failed':
            _, new_believe = rate(self.reference['connection_problem'], current)
        elif action == 'ping_succeeded':
            new_believe = current
            pass
        elif action == 'peerless_initial':
            new_believe, _ = rate(current, self.reference['already_known'])
            pass
        elif action == 'peerless_intro':
            new_believe, _ = rate(current, self.reference['new_data'])
            pass
        elif action == 'new_peer_discovered':
            new_believe = current
            pass
        elif action == 'new_block_id_found':
            new_believe, _ = rate(current, self.reference['new_data'])
        elif action == 'block_id_repeated':
            new_believe, _ = rate(current, self.reference['already_known'])
        elif action == 'loose_block':
            _, new_believe = rate(self.reference['bad_block'], current)
        elif action == 'block_supplied':
            new_believe = current
            pass
        elif action == 'block_not_supplied':
            _, new_believe = rate(self.reference['connection_problem'], current)
        else:
            raise Exception('Unknown code: {}'.format(action))

        self.believe[host] = new_believe

        return (self.believe[host].mu - 2*self.believe[host].sigma)/10

    def onPingFailed(self, host):
        say("PYCHAINNODE===> Ping failed to:", host)
        karma = self.update_believe(host, action='ping_failed')
        self.swarm.SetKarma(host, karma)
        self.inflight.discard(host)

    def onPingSucceeded(self, host):
        karma = self.update_believe(host, action='ping_succeeded')
        self.swarm.SetKarma(host, karma)
        self.inflight.discard(host)
        self.timeOfLastRemoteNewBlock = datetime.datetime.now()

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
            inflight = set()  # FIXME: inflight is not used.
        if host in self.inflight:
            return
        self.inflight.add(host)
        self.swarm.DoPing(host)

    def CreateSubscription(self, host):
        if host in self.subs:
            #say("AGENT_API PYCHAIN IGNORING ", host);
            self.SendPing(host)
            return

        self.subs.append(host)
        self.SendPing(host)
        self.swarm.DoDiscoverBlocks(host, 0)

        weightedSubList = [
            {
                'peer': x,
                'weight': self.swarm.GetKarma(x),
            }
            for x in self.subs
        ]

        weightedSubList.sort(key=lambda x: x['weight'])
        self.subs = [ x['peer'] for x in weightedSubList ]

        while len(self.subs) > self.maxpeers:
            x = self.subs.pop(0)
            say("AGENT_API PYCHAIN UNSUBSCRIBE ", x)
            self.swarm.DoStopBlockDiscover(x, 0)

        self.swarm.SetSitrep(json.dumps({
            "datatime": time.time(),
            "ident": self.swarm.queryOwnLocation(),
            "subscriptions": [
                {
                    'peer': x,
                    'weight': self.swarm.GetKarma(x),
                }
                for x in self.subs
            ]
        }))

        say("AGENT_API PYCHAIN SUBSCRIBE ", host)
        self.subs.append(host)

    def onIdle(self):
        goodPeers = self.swarm.GetPeers(self.maxpeers, 0)

        say("PYCHAINNODE===> idle1 ", len(goodPeers))
        goodPeers = [ x for x in goodPeers if x not in self.introductions ]

        say("PYCHAINNODE===> idle2 ", len(goodPeers))

        if not goodPeers:
            say("quiet")
            return 100

        weightedPeers = [(x,max(self.swarm.GetKarma(x), 0)) for x in goodPeers]
        total = sum([ x[1] for x in weightedPeers ])
        weight = random.random() * total

        weightedPeer = weightedPeers[0]
        while weight >= 0 and weightedPeers:
            weightedPeer = weightedPeers.pop(0)
            weight -= weightedPeer[1]

        host = weightedPeer[0]
        self.CreateSubscription(host)

        return 0

    def onPeerless(self):
        for peerListMember in self.peerlist:
            say("PYCHAIN initial peer", peerListMember)
            karma = self.update_believe(peerListMember, action='peerless_initial')
            self.swarm.SetKarma(peerListMember, karma)
            self.swarm.DoPing(peerListMember)
        for introListMember in self.introductions:
            karma = self.update_believe(introListMember, action='peerless_intro')
            self.swarm.SetKarma(introListMember, karma)

    def onNewPeerDiscovered(self, host):
        if host == self.swarm.queryOwnLocation():
            return
        #say("AGENT_API PYCHAIN NEW PEER ", host);
        karma = self.update_believe(host, action='new_peer_discovered')
        self.swarm.SetKarma(host, karma)

    def onNewBlockIdFound(self, host, blockid):
        say("AGENT_API PYCHAIN NEWBLOCK ", blockid)
        karma = self.update_believe(host, action='new_block_id_found')
        self.swarm.SetKarma(host, karma)
        self.timeOfLastRemoteNewBlock = datetime.datetime.now()

    def onBlockIdRepeated(self, host, blockid):
        # Awwww, we know about this.
        karma = self.update_believe(host, action='block_id_repeated')
        self.swarm.SetKarma(host, karma)

    def onLooseBlock(self, host, blockid):
        say("AGENT_API PYCHAIN LOOSE ", host, ' ', blockid)
        karma = self.update_believe(host, action='loose_block')
        self.swarm.SetKarma(host, karma)
        self.swarm.DoGetBlock(host, blockid)

    def onBlockSupplied(self, host, blockid):
        say("AGENT_API PYCHAIN DELIVERED ", host, ' ', blockid)
        karma = self.update_believe(host, action='block_supplied')
        self.swarm.SetKarma(host, karma)
        self.timeOfLastRemoteNewBlock = datetime.datetime.now()

    def onBlockNotSupplied(self, host, blockid):
        say("AGENT_API PYCHAIN FAILED  ", host, ' ', blockid)
        karma = self.update_believe(host, action='block_not_supplied')
        self.swarm.SetKarma(host, karma)


def run(config):
    agent = SwarmAgentBayesian(
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
    params.add_argument("-introduce",      default=True, help="Promote myself as a peer in order to join swarm.", action='store_true')
    params.add_argument("-chainident",     help="a number which represents which chain this is part of (for monitoring to use)", type=int, default=0)

    config = params.parse_args(sys.argv[1:])

    run(config)

if __name__ == "__main__":
    main()


DETAILS="""

lldb build/apps/pyfetch/pyfetch -- apps/pyfetch/pychainnode.py -id 10 -maxpeers 4 -target 16 -port 9010 -peers 127.0.0.1:9001,127.0.0.1:9005 -idlespeed 50 --introduce


(cd build;make -j100) && ./apps/demoswarm/runswarm.py --binary "./build/apps/pyfetch/pyfetch ./apps/pyfetch/pychainnode.py" --members 7 --maxpeers 3 --initialpeers 2 --idlespeed 50 --target 18

"""
