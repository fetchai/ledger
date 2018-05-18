import requests
import time
import json
import pdb
import random
import optparse

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

#setRate             = 100000
#transactionsPerCall = 1
setRate             = 10
minRate             = 10
sleepTime           = 20
transactionsPerCall = { "transactions": 10 }

### localhost test
#endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "localhost"}

## LAN test
#endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "192.168.1.213"}

# pi test
#endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "192.168.1.150"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "192.168.1.151"}

# google cloud test
endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.204.38.91"}
endpoint2 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "35.204.60.187"}

HTTPpost(endpoint1, 'set-transactions-per-call', transactionsPerCall)
HTTPpost(endpoint2, 'set-transactions-per-call', transactionsPerCall)
print "Set trans/call to ", transactionsPerCall["transactions"]
#sleep(80)

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

    rate = { "rate": setRate }
    HTTPpost(endpoint1, 'set-rate', rate)
    HTTPpost(endpoint2, 'set-rate', rate)
    print "Set rate to: ", setRate

    HTTPpost(endpoint1, 'set-transactions-per-call', transactionsPerCall)
    HTTPpost(endpoint2, 'set-transactions-per-call', transactionsPerCall)
    print "Set trans/call to ", transactionsPerCall["transactions"]

    HTTPpost(endpoint1, 'start')
    #HTTPpost(endpoint2, 'start')

    print "Sleeping for: ", sleepTime
    time.sleep(sleepTime)

    print "Finished sleeping. Stopping."

    HTTPpost(endpoint1, 'stop')
    HTTPpost(endpoint2, 'stop')

    print "Stopped"
    time.sleep(4)

    ## inspect the hashes (not generally recommended)
    #page1 = HTTPpost(endpoint1, 'transactions')
    #print "Result", jsonPrint(page1)
    #exit(1)

    print "Inspecting hash"
    page1 = HTTPpost(endpoint1, 'transactions-hash')
    page2 = HTTPpost(endpoint2, 'transactions-hash')

    wantHashMatch = False
    if((ordered(page1.json())) != (ordered(page2.json()))):
        print "FAILED TO MATCH: "
        print "node 0", jsonPrint(page1)
        print "node 1", jsonPrint(page2)
        if(wantHashMatch):
          setRate = setRate*2
          continue
          #exit(1)
    else:
        print "Successfully matched"

    print "RX: ", page1.json()["numberOfTransactions"]
    print "TX: ", page2.json()["numberOfTransactions"]
    numberOfTransactions = min(page1.json()["numberOfTransactions"], page2.json()["numberOfTransactions"])

    #print jsonPrint(page1)
    print ">Number of transactions: ", numberOfTransactions
    transPerSecond = numberOfTransactions/sleepTime
    print ">Transactions per second: ", transPerSecond

    print "=>", transactionsPerCall["transactions"], "\t", transPerSecond

    if(transPerSecond > transPerSecondMax):
        transPerSecondMax = transPerSecond

    print ">Transactions per second max: ", transPerSecondMax
    print

    number = transactionsPerCall["transactions"]
    transactionsPerCall["transactions"] = number*2
