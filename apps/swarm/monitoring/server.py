#!/usr/bin/env python3

import bottle
import datetime
import functools
import glob
import json
import os
import pprint
import math
import re
import socket
import sqlite3
import subprocess
import sys
import contextlib
import random

from monitoring.Monitoring import Monitoring

from bottle import route, run, error, template, hook, request, response, static_file, redirect

SIZE_OPACITY_HISTORY_SCALES = [
    ( 45, 1.0, 1.0, ),
    ( 30, .50, 0.4),
    ( 13, .20, 0.3),
    ( 7,  .10, 0.15),
    ( 4,  .05, 0.07),
    ( 1,  .03, 0.03)
]

MAX_DEPTH = len(SIZE_OPACITY_HISTORY_SCALES)-1

me = os.path.dirname(os.path.realpath(__file__))
sys.path.append(me)

from utils import flags

def getStaticFilePath(filepath):
    for i in [
            './static/',
    ]:
        fn = os.path.join(str(i), str(filepath))
        if os.path.exists(fn):
            return (i, filepath)

def get_static(filepath):
    x = getStaticFilePath(filepath)
    return static_file(filepath, root=x[0])

def get_data(context):
    fp = getStaticFilePath("dummydata.json")
    fp = os.path.join(fp[0], fp[1])
    with open(fp, "r") as fh:
        data = json.loads("\n".join(fh.readlines()))
    return data

def get_data2(context, mon):

    data = {
        "nodes": [],
        "links": [],
    }

    allnodenames = set([x for x in mon.world.keys()])
    extranodenames = set()

    for s in mon.world.keys():
        for link in mon.world[s]["peers"]:
            t = link["peer"]
            w = link["weight"]
            if not t:
                continue
            if w > 0.1:
                data["links"].append({
                    "value": w,
                    "source": s,
                    "target": t,
                })
                if t not in allnodenames:
                    extranodenames.add(t)

    allnodenames = list(allnodenames)
    extranodenames = list(extranodenames)

    data["nodes"].extend([
            {
                "id": x,
                "group": mon.world[x]["state"],
                "status": "ok",
            } for x in allnodenames
        ]),

    data["nodes"].extend([
            {
                "id": x,
                "group": 0,
                "status": "dead",
            } for x in extranodenames
        ]),
    return data;

def get_chain_data2(context, mon):
    r = []

    if not mon.chain.keys():
        return {
            'nodes': []
        }

    chainident = max(mon.chain.keys())
    blocks = mon.chain.get(chainident, {})
    if not blocks:
        return {
            'nodes': []
        }

    allblocknames = list(blocks.keys())
    blocknameToId = dict((name, index+1) for index,name in enumerate(allblocknames))
    blocknameToId[""] = 0

    for name in allblocknames:
        b = blocks[name]

        prev_name = b.get('prev', "")
        prev_id = blocknameToId.get(prev_name, 0)
        id = blocknameToId.get(name)

        r.append({
            'name': {
                'v': str(id),
                'f': """
<div style="{}">{}</div>
<div>
<span>blk:</span>&nbsp;<span style="font-weight:bold">{}</span>
<span>by:</span>&nbsp;<span style="font-weight:bold">{}</span>
</div>
<div>
<span style="font-weight:bold">{} &mdash;</span>
<span style="font-style:italic">{}</span>
</div>
                """.format(
                    "color: red;" if name in mon.heaviests.values() else "",
                    str(name)[0:16],
                    b["num"],
                    b["miner"],
                    len(b["nodes"]),
                    ",".join([ x[-1] for x in sorted(b["nodes"])])),
            },
            'manager': str(prev_id),
            'tooltip': '',
        })

    if any([ node['manager'] == '0' for node in r ]):
        r.append({
            'name': {
                'v': '0',
                'f': 'detached',
            },
            'manager': None,
            'tooltip': ''
        })

    return {
        'nodes': r
    }

def get_chain_data(context, mon):
    r = {
        'nodes': [],
        'links': []
    }

    allblocknames = list(mon.chain.keys())

    for name in allblocknames:
        b = mon.chain[name]
        r['nodes'].append({
            'id': str(b["id"]),
            'group': 1,
            'status': 1,
        })

    for name in allblocknames:
        b = mon.chain.get(name, {})
        prevHash = b.get("prev", "")
        p = mon.chain.get(prevHash, None)

        if p:
            r['links'].append({
                'source': str(b['id']),
                'target': str(p['id']),
                'value': len(b['nodes']),
            })

    return r

# Returns list of (depth, name, prev)
def get_ancestry(chain, blockname, maxdepth, offs=0):
    r = []

    i = 0
    while (i <= maxdepth) and blockname:
        b = chain.get(blockname, {})
        p = b.get("prev", "")
        r.append(
            (i+offs, blockname, p,)
        )
        blockname = p
        i += 1
    return r


class CurrentNodesModel(object):
    def __init__(self):
        self.nodes = {}
        self.oldheads = set()

    def clear(self):
        self.nodes = {}
        self.oldheads = set()

    def get(self, name):
        return self.nodes.get(name, {})

    def add(self, name, newnode):
        if name in self.nodes:
            if newnode["depth"] > self.nodes[name]['depth']:
                return
        self.nodes[name] = newnode

    def yieldNodes(self):
        for k,v in self.nodes.items():
            yield v

    def has(self, name):
        return name in self.nodes

    def populate(self, chain, currentheads):
        self.nodes = {}

        currentHeadsWithBlockNumbers = [ ( chain[name]["num"], name )
                  for name in currentheads
                  if name in chain
        ]

        if not currentHeadsWithBlockNumbers:
            return

        currentHeadsInDecreasingBlockNumber = list(reversed(
            sorted(
                currentHeadsWithBlockNumbers,
                key=lambda x: x[0]
            )
        ))

        shallowest_num = currentHeadsInDecreasingBlockNumber[0][0]
        newoldheads = set()

        for num, name in currentHeadsInDecreasingBlockNumber:
            offs=shallowest_num - num
            ancs = get_ancestry(chain, name, MAX_DEPTH, offs=offs)
            ancs = list(filter(lambda x: x[0]<=MAX_DEPTH, ancs))

            for foo in ancs:
                depth= foo[0]
                name = foo[1]
                prev = foo[2]
                self.add(name, {
                    'depth': depth,
                    'name' : name,
                    'prev' : prev,
                })
                newoldheads.add(name)

        for headname in self.oldheads:
            if headname not in chain:
                continue
            num = chain[headname]["num"]
            ancs = get_ancestry(chain, headname, MAX_DEPTH, offs=shallowest_num - num)
            ancs = list(filter(lambda x: x[0]<=MAX_DEPTH, ancs))
            if not any([ name in self.nodes for depth, name, prev in ancs]):
                continue
            if not ancs:
                continue
            newoldheads.add(headname)

            foo = [
                (name, {
                    'depth': depth,
                    'name': name,
                    'prev': prev,
                })
                for depth, name, prev
                in ancs
            ]
            for a in foo:
                self.add(a[0], a[1])

        self.oldheads = newoldheads


currentNodesModel = CurrentNodesModel()

def get_consensus_data(context, mon):
    r = {
        'nodes': [],
        'links': [],
    }

    myChain = mon.getChain()
    currentNodesModel.populate(myChain, mon.heaviests.values())


    for node in currentNodesModel.yieldNodes():
        d = min(node['depth'], MAX_DEPTH)
        s = SIZE_OPACITY_HISTORY_SCALES[d]
        k = node['name']
        step = MAX_DEPTH - d

        n = {
            'id': k[0:16],
            'group': ord((k or "0")[0]) & 0x0F,
            'value': s[0],
            'charge': -1500,
            'status': "darken",
            'class': "BLOCK",
            'opacity': s[1],
            'inherit': myChain.get(k, {}).get("prev", "")[0:16],
        }
        r['nodes'].append(n)


    r["nodes"].extend([
        {
            'id': k,
            'group': ord((attracted or "0")[0]) & 0x0F,
            'charge': -300,
            'value': 10,
        } for k, attracted in mon.heaviests.items()
    ])



    for node in currentNodesModel.yieldNodes():
        p = node['prev']
        name = node['name']
        step = node['depth']
        s = SIZE_OPACITY_HISTORY_SCALES[step]
        if currentNodesModel.has(p):
            r["links"].extend([{
                    "source": p[0:16],
                    "target": name[0:16],
                    "value": s[1] * 3,
                    'distance': 100 * s[2],
            }])

    r["links"].extend([
        {
            'source': node,
            'target': heaviest[0:16],
            'value': 0,
            'distance': 0.01,
        }
        for node, heaviest in mon.heaviests.items()
    ])
    return r


def get_slash():
    redirect("/static/monitor.html")

flags.Flag(g_port = "port", help = "Which port to run on", required = True)
flags.Flag(g_ssl = "ssl", type=bool, help = "Run https", default = False)
flags.Flag(g_certfile = "cert", help = "Certificate", default = None)

def main():
    flags.startFlags(sys.argv)

    #bottle.debug(True)

    context = {}

    root = bottle.Bottle()
    root.route('/static/<filepath:path>', method='GET', callback=functools.partial(get_static))
    root.route('/data', method='GET', callback=functools.partial(get_data, context))
    root.route('/', method='GET', callback=functools.partial(get_slash))

    with contextlib.closing(Monitoring()) as myMonitoring:
        root.route('/network', method='GET', callback=functools.partial(get_data2, context, myMonitoring))
        root.route('/chain', method='GET', callback=functools.partial(get_chain_data, context, myMonitoring))
        root.route('/chain2', method='GET', callback=functools.partial(get_chain_data2, context, myMonitoring))
        root.route('/consensus', method='GET', callback=functools.partial(get_consensus_data, context, myMonitoring))
        if g_ssl:
            from utils import SSLWSGIRefServer
            srv = SSLWSGIRefServer.SSLWSGIRefServer(host="0.0.0.0", port=g_port)
            bottle.run(server=srv, app=root)
        else:
            bottle.run(app=root, host='0.0.0.0', port=g_port)

if __name__ == "__main__":
    main()
