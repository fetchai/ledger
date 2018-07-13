#!/usr/bin/env python3

import bottle
import datetime
import functools
import glob
import json
import os
import pprint
import re
import socket
import sqlite3
import subprocess
import sys
import contextlib
import random

from monitoring.Monitoring import Monitoring

from bottle import route, run, error, template, hook, request, response, static_file, redirect

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
                print("-----------------------------------")
                print(s, link)
                print("-----------------------------------")
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
    print(data)
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
{}
<div>
<span>blk:</span>&nbsp;<span style="font-weight:bold">{}</span>
<span>by:</span>&nbsp;<span style="font-weight:bold">{}</span>
</div>
<div>
<span style="font-weight:bold">{} &mdash;</span>
<span style="font-style:italic">{}</span>
</div>
                """.format(
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
        b = mon.chain[name]
        prevHash = b.get("prev", "")
        p = mon.chain.get(prevHash, None)

        if p:
            r['links'].append({
                'source': str(b['id']),
                'target': str(p['id']),
                'value': len(b['nodes']),
            })

    print(r)
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
        if g_ssl:
            from utils import SSLWSGIRefServer
            srv = SSLWSGIRefServer.SSLWSGIRefServer(host="0.0.0.0", port=g_port)
            bottle.run(server=srv, app=root)
        else:
            bottle.run(app=root, host='0.0.0.0', port=g_port)

if __name__ == "__main__":
    main()
