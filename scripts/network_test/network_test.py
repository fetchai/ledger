import requests
import time
import json
import pdb
import random

#import random as rand
import sys, os

# print json in pretty way
def jsonPrint(r):
    return json.dumps(r.json(), indent=4, sort_keys=True)+"\n"

#https://stackoverflow.com/questions/25851183/
def ordered(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered(x) for x in obj)
    else:
        return obj

def HTTPpost(endpoint, page, jsonArg="{}"):
    return requests.post('http://'+str(endpoint["IP"])+':'+str(endpoint["HTTPPort"])+'/'+page, json=jsonArg)

setRate = 900
minRate = 0
sleepTime = 5

# localhost test
endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "localhost"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "localhost"}

# pi test
#endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "192.168.1.150"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "192.168.1.151"}


HTTPpost(endpoint1, 'add-endpoint', endpoint2)
HTTPpost(endpoint2, 'add-endpoint', endpoint1)

transPerSecondMax = 0

# Loop the test
for i in range(1000):
    print "Starting"
    HTTPpost(endpoint1, 'stop')
    HTTPpost(endpoint2, 'stop')

    HTTPpost(endpoint1, 'reset')
    HTTPpost(endpoint2, 'reset')

    #requests.post('http://localhost:8081/start')
    #requests.post('http://localhost:8080/start')

    rate = { "rate": setRate }
    HTTPpost(endpoint1, 'set-rate', rate)
    HTTPpost(endpoint2, 'set-rate', rate)
    print "Set rate to: ", setRate

    HTTPpost(endpoint1, 'start')
    #HTTPpost(endpoint2, 'start')

    print "Sleeping..."
    time.sleep(sleepTime)

    print "Finished sleeping. Stopping."

    HTTPpost(endpoint1, 'stop')
    HTTPpost(endpoint2, 'stop')

    print "Stopped"
    time.sleep(2)

    page1 = HTTPpost(endpoint1, 'transactions-hash')
    page2 = HTTPpost(endpoint2, 'transactions-hash')

    if((ordered(page1.json())) != (ordered(page2.json()))):
        print "FAILED TO MATCH: "
        print "node 0", jsonPrint(page1)
        print "node 1", jsonPrint(page2)
        exit(1)
    else:
        print "Successfully matched"

    #print jsonPrint(page1)
    print ">Number of transactions: ", page2.json()["numberOfTransactions"]
    transPerSecond = page2.json()["numberOfTransactions"]/sleepTime
    print ">Transactions per second: ", transPerSecond

    if(transPerSecond > transPerSecondMax):
        transPerSecondMax = transPerSecond

    print ">Transactions per second max: ", transPerSecondMax
    print

    # Set new transaction rate
    setRate = int(setRate/2) -1
    if(setRate < minRate):
        setRate = minRate
