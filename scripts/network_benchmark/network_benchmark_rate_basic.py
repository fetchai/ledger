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


# Will act as controlling node if used
masterEndpoint = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
usingMasterNode = False

# localhost test
# endpoint1       = {"HTTPPort": 8083, "TCPPort": 9083, "IP": "localhost"}
# endpoint2       = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "localhost"}
# endpoint3       = {"HTTPPort": 8082, "TCPPort": 9082, "IP": "localhost"}
# allEndpoints    = [ endpoint1, endpoint2, endpoint3]
# activeEndpoints = [ endpoint1, endpoint2, endpoint3]

# cloud test basic
# endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.204.38.91"}
# endpoint2 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.204.60.187"}
# allEndpoints = [ endpoint1, endpoint2]

# google cloud test
endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.204.38.91"}
endpoint2 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.204.60.187"}
endpoint3 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.234.64.165"}
endpoint4 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.234.132.50"}
# endpoint5 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.227.63.152"} # America
allEndpoints = [endpoint1, endpoint2, endpoint3, endpoint4]
# Endpoints that are transmitting
activeEndpoints = [
    endpoint1,
    endpoint2,
    endpoint3,
    endpoint4
]

# Global config
numberOfNodes = len(allEndpoints)
txSize = 2000  # bytes
txPerCall = 10000
txToSync = txPerCall * 15
totalSize = txToSync * numberOfNodes * txSize
print "Total size on each node after sync: ", totalSize / 1000000, " MB"

transactionSize = {"transactionSize": txSize}
transactionsPerCall = {"transactions": txPerCall}
transactionsToSync = {"transactionsToSync": txToSync}
stopCondition = {
    "stopCondition": int(
        txToSync /
        txPerCall) *
    len(activeEndpoints)}

for endpoint in allEndpoints:
    HTTPpost(endpoint, 'reset')

# Set up connections to each other (circular topology)
# for i in range(len(allEndpoints)):
#    if (i == len(allEndpoints)-1):
#        HTTPpost(allEndpoints[i], 'add-endpoint', allEndpoints[0])
#    else:
#        HTTPpost(allEndpoints[i], 'add-endpoint', allEndpoints[i+1])

# Fully connected topology
for endpoint in allEndpoints:
    for otherEndpoint in allEndpoints:
        if(endpoint != otherEndpoint):
            HTTPpost(endpoint, 'add-endpoint', otherEndpoint)

# Other setup parameters
for endpoint in allEndpoints:
    HTTPpost(endpoint, 'transaction-size', transactionSize)
    HTTPpost(endpoint, 'transactions-per-call', transactionsPerCall)
    HTTPpost(endpoint, 'stop-condition', stopCondition)

for endpoint in activeEndpoints:
    HTTPpostAsync(endpoint, 'transactions-to-sync', transactionsToSync)

# Need to make sure everyone is ready before starting
for t in threads:
    t.join()

# master has all endpoints, set it up
if (usingMasterNode):
    for endpoint in allEndpoints:
        HTTPpost(endpoint, 'is-slave', "{}")
        HTTPpost(masterEndpoint, 'add-endpoint', endpoint)

epoch_time = int(time.time())
timeWait = 3
threeSecondsTime = {"startTime": epoch_time + timeWait}

# Set up the start time
for endpoint in allEndpoints:
    HTTPpostAsync(endpoint, 'start-time', threeSecondsTime)

if (usingMasterNode):
    HTTPpostAsync(masterEndpoint, 'start-test-as-master', threeSecondsTime)

time.sleep(3)

# wait until they're probably done
while(True):
    time.sleep(7)
    hashPages = [HTTPpost(i, 'finished').json()["finished"] == True
                 for i in allEndpoints]
    print "Finished : ", hashPages
    if(sum(hashPages) == len(hashPages)):
        break

# Get the time each node took to synchronise
pages = []
maxTime = 0

for endpoint in allEndpoints:
    pageTemp = HTTPpost(endpoint, 'time-to-complete')
    print jsonPrint(pageTemp)
    if(pageTemp.json()["timeToComplete"] > maxTime):
        maxTime = pageTemp.json()["timeToComplete"]
    pages += pageTemp

if (usingMasterNode):
    pageTemp = HTTPpost(masterEndpoint, 'time-to-complete')
    print jsonPrint(pageTemp)
    if(pageTemp.json()["timeToComplete"] > maxTime):
        maxTime = pageTemp.json()["timeToComplete"]
    pages += pageTemp

print "Max time: ", maxTime
TPS = (txToSync * numberOfNodes) / maxTime
print "Transactions per second: ", TPS
print "Transactions per second per node: ", TPS / numberOfNodes
print "Mbits/s", (TPS * txSize * 8) / 1000000
print "Mbits/s per node", (TPS * txSize * 8) / 1000000 / numberOfNodes

exit(1)

# Check that they have synchronised correctly
print "inspecting the hashes (may take a long time)"
hashPages = []

hashes = [ordered(HTTPpost(i, 'transactions-hash').json())
          for i in allEndpoints]
# hashes = [1, 1]
comparison = [x == hashes[0] for x in hashes]

if(all(comparison) == False):
    print "FAILED TO MATCH: "
    print hashes
    res = [HTTPpost(i, 'transactions').json() for i in allEndpoints]
    for r in res:
        for i in r:
            print i
        print ""
else:
    print "Hashes matched!"
    print hashes[0]
