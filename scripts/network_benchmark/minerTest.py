import requests
import time
import json
import pdb
import random
import optparse
import threading
import sys
import os


def jsonPrint(r):
    """Pretty-print JSON"""
    return json.dumps(r.json(), indent=4, sort_keys=True) + "\n"


def ordered(obj):
    """Order map for comparison

    https://stackoverflow.com/questions/25851183/
    """
    if isinstance(obj, dict):
        return sorted((k, ordered(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered(x) for x in obj)
    else:
        return obj


def HTTPpost(endpoint, page, jsonArg="{}"):
    """Make HTTP post"""
    return requests.post('http://{ip}:{port}/{page}'.format(
        ip=endpoint["IP"], port=endpoint["HTTPPort"], page=page), json=jsonArg)


threads = []


def HTTPpostAsync(endpoint, page, jsonArg="{}"):
    thread = threading.Thread(target=HTTPpost, args=(endpoint, page, jsonArg))
    thread.start()
    threads.append(thread)


# localhost test
endpoint0 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
endpoint1 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "localhost"}
endpoint2 = {"HTTPPort": 8082, "TCPPort": 9082, "IP": "localhost"}
endpoint3 = {"HTTPPort": 8083, "TCPPort": 9083, "IP": "localhost"}
endpoint4 = {"HTTPPort": 8084, "TCPPort": 9084, "IP": "localhost"}
allEndpoints = [endpoint0, endpoint1, endpoint2, endpoint3, endpoint4]
#allEndpoints    = [ endpoint0, endpoint1 ]

# Set each nodes connection to each other

# Fully connected topology
for endpoint in allEndpoints:
    for otherEndpoint in allEndpoints:
        if endpoint != otherEndpoint:
            HTTPpost(endpoint, 'add-endpoint', otherEndpoint)

print "starting"

# Start all the miners
for endpoint in allEndpoints:
    HTTPpost(endpoint, 'start')

print "waiting"

# Ten seconds of mining
time.sleep(8)

print "resetting a miner"
HTTPpost(endpoint0, 'reset')

time.sleep(8)

print "stopping miners"

# stop all the miners
for endpoint in allEndpoints:
    HTTPpostAsync(endpoint, 'stop')
