import os
import requests
import re
import sys
import time
import threading
import json

from multiprocessing import Pool
from monitoring import Getter

from utils.messages import title, note, text, info, debug, progress, warn, error, fatal

from utils import flags
from utils import resources

flags.Flag(g_sitrepfile = flags.AUTOFLAG, help = "File to write log of all sitreps to", default = None)

def workfunc(ident):
    return (
        poll(ident),
        poll2(ident),
        )

class PyfetchNodeNumberGenerator(object):
    def __init__(self, limit=25):
        self.limit = limit

    def getall(self):
        for x in range(0, self.limit):
            yield {
                "ident": x,
                "port": 9000+x,
            }

class ConstellationNodeNumberGenerator(object):
    def __init__(self, limit=10):
        self.limit = limit

    def getall(self):
        for x in range(0, self.limit):
            yield {
                "ident": x,
                "port": 9000+x*20,
            }

class NodesFromFileGenerator(object):
    def __init__(self, fn):
        self.fn = fn

    def getall(self):
        with open(self.fn, "r") as fh:
            for id,line in enumerate(fh.readlines()):
                yield {
                    "ident": id,
                    "url": line.rstrip(),
                }

class Monitoring(object):
    def __init__(self, nodenumbergenerator):
        self.getter = Getter.Getter(
            self,
            nodenumbergenerator,
            {
                '/mainchain': self.newChainData,
                '/sitrep': self.newSitrep,
                200: self.okPeer,
                None: self.deadPeer,
            }
            )
        self.getter.start()

        self.world = {}
        self.heaviests = {}
        self.chain = {}
        self.deadPeers = {}

        self.sitreps = open(g_sitrepfile,  'w') if g_sitrepfile else None

    def newSitrep(self, nodenumber, ident, url, code, data):
        if data == None:
            return
        peerlist = data["subscriptions"]
        ident = data.get("ident", ident)
        self.setPeerList(nodenumber, ident, peerlist)
        if self.sitreps:
            self.sitreps.write(json.dumps(data))
            self.sitreps.write("\n\n")
        if "chain" in data:
            self.newChainData(nodenumber, ident, url, code, data["chain"])

    def newChainData(self, nodenumber, ident, url, code, data):
        blocks = data["blocks"]
        chainident = data.get("chainident", 0)

        if self.chain.keys():
            if chainident < max(self.chain.keys()):
                return
            if chainident > max(self.chain.keys()):
                self.chain = {}

        for i, block in enumerate(blocks):
            if not i:
                self.heaviests[ident] = block["hashcurrent"]
            self.chain.setdefault(chainident, {})
            self.chain[chainident].setdefault(block["hashcurrent"], { 'id': len(self.chain)+1 })
            self.chain[chainident][block["hashcurrent"]]["prev"] = block["hashprev"]
            self.chain[chainident][block["hashcurrent"]].setdefault("nodes", set())
            self.chain[chainident][block["hashcurrent"]]["nodes"].add(ident)
            self.chain[chainident][block["hashcurrent"]]["num"] = block["blockNumber"]
            self.chain[chainident][block["hashcurrent"]]["miner"] = block["minerNumber"]

    def complete(self):
        if not self.sitreps:
            return
        sitrep = {
            "datatime": time.time(),
            "headcount": len(set(self.heaviests.values())),
            "heads": {
            },
            "nodecount": len(set(self.heaviests.keys())),
        }

        for k,v in self.heaviests.items():
            sitrep['heads'][v] = sitrep['heads'].get(v, 0) + 1

        self.sitreps.write(json.dumps(sitrep))
        self.sitreps.write("\n\n")
        self.sitreps.flush()

    def getChain(self):
        if len(self.chain):
            return self.chain[max(self.chain.keys())]
        return {}

    def deadPeer(self, nodenumber, ident, url, code, data):
        self.deadPeers[ident] = 1 + self.deadPeers.get(ident, 0)
        if self.deadPeers[ident] > 20:
            self.badNode(ident)

    def okPeer(self, nodenumber, ident, url, code, data):
        self.deadPeers.pop(ident, None)

    def close(self):
        self.getter.stop()
        if self.sitreps:
            self.sitreps.cloxse()

    def setPeerList(self, nodenumber, ident, peerlist):
        self.world.setdefault(ident, {})
        self.world[ident].setdefault("peers", [])
        self.world[ident]["peers"] = peerlist
        self.world[ident]["label"] = nodenumber

    def newData(self, nodenumber, ident, url, code, data):
        peerlist = data["peers"];
        self.setPeerList(ident, nodenumber, peerlist)

    def badNode(self, ident):
        self.world.pop(ident, None)
        self.heaviests.pop(ident, None)
