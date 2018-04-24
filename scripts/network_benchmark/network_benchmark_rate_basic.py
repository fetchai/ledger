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

# Config
setRate             = { "rate" : 0 }
transactionsToSync  = { "transactionsToSync": 60000 }
transactionsPerCall = { "transactions": 5000 }
stopCondition       = { "stopCondition": 600000 }

## localhost test
#endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "localhost"}

## LAN test
#endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "192.168.1.213"}

## pi test
#endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "192.168.1.150"}
#endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "192.168.1.151"}

# google cloud test
endpoint1 = {"HTTPPort": 8080, "TCPPort": 9080, "IP": "localhost"}
endpoint2 = {"HTTPPort": 8081, "TCPPort": 9081, "IP": "35.204.5.118"}

HTTPpost(endpoint1, 'reset')
HTTPpost(endpoint2, 'reset')

# Set up connections to each other
HTTPpost(endpoint1, 'add-endpoint', endpoint2)
HTTPpost(endpoint2, 'add-endpoint', endpoint1)

# Other setup parameters
HTTPpost(endpoint1, 'set-rate', setRate)
HTTPpost(endpoint2, 'set-rate', setRate)
HTTPpost(endpoint1, 'set-transactions-per-call', transactionsPerCall)
HTTPpost(endpoint2, 'set-transactions-per-call', transactionsPerCall)

# Set up the transactions we want to sync with each other
#HTTPpost(endpoint1, 'transactions-to-sync', transactionsToSync)
HTTPpost(endpoint2, 'transactions-to-sync', transactionsToSync)

# Set up the stop condition
HTTPpost(endpoint1, 'stop-condition', stopCondition)
HTTPpost(endpoint2, 'stop-condition', stopCondition)

epoch_time       = int(time.time())
timeWait = 5
threeSecondsTime = { "startTime": epoch_time+timeWait }
# Set up the start time
HTTPpost(endpoint1, 'start-time', threeSecondsTime)
#HTTPpost(endpoint2, 'start-time', threeSecondsTime)

# wait until they're probably done
while(HTTPpost(endpoint1, 'finished').json()["finished"] == 0 or HTTPpost(endpoint1, 'finished').json()["finished"] == 0):
    time.sleep(10)

# Check that they have syncronised correctly
print "inspecting the hash"
page1 = HTTPpost(endpoint1, 'transactions-hash')
page2 = HTTPpost(endpoint2, 'transactions-hash')

if((ordered(page1.json())) != (ordered(page2.json()))):
    print "FAILED TO MATCH: "
    print "node 0", jsonPrint(page1)
    print "node 1", jsonPrint(page2)
    #exit(1)
else:
    print "Hashes matched!"

# Get the time each node took to syncronise
page1 = HTTPpost(endpoint1, 'time-to-complete')
page2 = HTTPpost(endpoint2, 'time-to-complete')

print jsonPrint(page1)
print jsonPrint(page2)

maxTime = page1.json()["timeToComplete"]
if(page2.json()["timeToComplete"] > maxTime):
    maxTime = page2.json()["timeToComplete"]

print "Max time: ", maxTime
print "Transactions per second: ", (stopCondition["stopCondition"]/2)/maxTime

