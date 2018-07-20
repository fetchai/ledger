import os
import requests
import re
import sys
import time
import threading
import json

from multiprocessing import Pool
from monitoring import Getter

def workfunc(ident):
    return (
        poll(ident),
        poll2(ident),
        )

class NodeNumberGenerator(object):
    def __init__(self):
        pass

    def getall(self):
        for x in range(0, 25):
            yield x

class Monitoring(object):
    def __init__(self):
        print("MONITORING START?")
        self.getter = Getter.Getter(
            NodeNumberGenerator(),
            {
                '/peers': self.newData,
                '/mainchain': self.newChainData,
            }
            )
        self.getter.start()

        self.world = {}
        self.heaviests = {}
        self.chain = {}

    def newChainData(self, nodenumber, ident, url, code, data):
        blocks = data["blocks"]
        chainident = data["chainident"]

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

    def getChain(self):
        if len(self.chain):
            return self.chain[max(self.chain.keys())]
        return {}

    def close(self):
        self.thread.done = True
        self.thread.join(100)

    def newData(self, nodenumber, ident, url, code, data):
        peers = data["peers"];

        self.world.setdefault(ident, {})
        self.world[ident].setdefault("peers", [])
        self.world[ident]["peers"] = peers

    def badNode(self, ident):
        self.world.pop(ident, None)

