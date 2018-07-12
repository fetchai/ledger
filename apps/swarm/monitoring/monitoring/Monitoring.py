import os
import requests
import re
import sys
import time
import threading
import json

POSSIBLE_PORTS = 30

class Monitoring(object):

    class WorkerThread(threading.Thread):
        def __init__(self, owner):
            self.done = False
            self.owner = owner
            self.port = 0
            super().__init__(group=None, target=None, name="pollingthread")

        def run(self):
            print("MONITORING START")
            while not self.done:
                self.poll(self.port + 10000, self.port)
                self.poll2(self.port + 10000, self.port)
                self.port = (self.port + 1) % POSSIBLE_PORTS
                if self.port == 0:
                    time.sleep(2)

        def poll(self, port, nodenumber):
            ident = "127.0.0.1:{}".format(nodenumber + 9000)
            try:
                url = "http://127.0.0.1:{}/peers".format(port)
                data = None
                try:
                    r = requests.get(url, timeout=1)
                    if r.status_code == 200:
                        data = json.loads(r.content.decode("utf-8", "strict"))
                        peers = data.get("peers", [])
                        state = data.get("state", 0)
                except requests.exceptions.Timeout as ex:
                    data = None
                    print("Timeout:", ident)
                except requests.exceptions.ConnectionError as ex:
                    data = None
                    print("Denied:", ident)

                if data != None:
                   self.owner.newData(ident, peers, state)
                else:
                    self.owner.badNode(ident)

            except Exception as x:
                print("ERR:", x)

        def poll2(self, port, nodenumber):
            ident = "127.0.0.1:{}".format(nodenumber + 9000)
            try:
                url = "http://127.0.0.1:{}/mainchain".format(port)
                data = None
                try:
                    r = requests.get(url, timeout=1)
                    if r.status_code == 200:
                        data = json.loads(r.content.decode("utf-8", "strict"))
                except requests.exceptions.Timeout as ex:
                    data = None
                    print("Timeout:", ident)
                except requests.exceptions.ConnectionError as ex:
                    data = None
                    print("Denied:", ident)

                if data != None:
                   self.owner.newChainData(ident, data["blocks"], data["chainident"])
                else:
                    self.owner.badNode(ident)

            except Exception as x:
                print("ERR:", x)


    def __init__(self):
        print("MONITORING START?")
        self.thread = Monitoring.WorkerThread(self)
        self.thread.start()

        self.world = {}
        self.heaviests = {}
        self.chain = {}

    def newChainData(self, ident, blocks, chainident):

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

    def newData(self, ident, peers, state):
        self.world.setdefault(ident, {})
        self.world[ident].setdefault("peers", [])
        self.world[ident].setdefault("state", 0)

        self.world[ident]["state"] = state
        self.world[ident]["peers"] = peers

    def badNode(self, ident):
        self.world.pop(ident, None)

