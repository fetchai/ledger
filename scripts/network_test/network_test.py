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

endpoint = {"TCPPort": 9081, "IP": "localhost"}
requests.post('http://localhost:8080/add-endpoint', json=endpoint)

#endpoint = {"TCPPort": 9080, "IP": "localhost"}
#requests.post('http://localhost:8081/add-endpoint', json=endpoint)

## generate an/some events by selecting a random aea
#for i in range(1, workingNodes-1):
#    connection = {"TCPPort": 9080+i, "IP": "localhost"}
#    requests.post('http://localhost:8080/add-connection', json=connection)
#

setRate = 100
rate = { "rate": setRate }
r = requests.post('http://localhost:8080/set-rate', json=rate)
jsonPrint(r)

r = requests.post('http://localhost:8081/set-rate', json=rate)
jsonPrint(r)

requests.post('http://localhost:8080/start')

sleepTime = 10000
time.sleep(sleepTime)

requests.post('http://localhost:8080/stop')
#requests.post('http://localhost:8081/stop')

hash1 = requests.post('http://localhost:8080/transactions-hash')
hash2 = requests.post('http://localhost:8081/transactions-hash')

print "hash1: ", hash1.json()["hash"]
print "hash2: ", hash2.json()["hash"]

page1 = requests.post('http://localhost:8080/transactions')
page2 = requests.post('http://localhost:8081/transactions')

print "Number of transactions: ", len(page1.json())
print "Transactions per second: ", len(page1.json())/sleepTime

#print jsonPrint(page1)
#print ""
#print ""
#print jsonPrint(page2)

requests.post('http://localhost:8080/stop')
#requests.post('http://localhost:8081/stop')

#if(ordered(page1.json()) != ordered(page2.json())):
if((page1.json()) != (page2.json())):
    print "FAILED TO MATCH: "
    print jsonPrint(page1)
    print jsonPrint(page2)
else:
    print "Successfully matched"

# Loop the test
for i in range(1000):
    requests.post('http://localhost:8080/reset')
    requests.post('http://localhost:8081/reset')

    #requests.post('http://localhost:8081/start')
    requests.post('http://localhost:8080/start')

    sleepTime = 2
    time.sleep(sleepTime)

    requests.post('http://localhost:8080/stop')

    setRate = setRate - 100
    if(setRate < 0):
        setRate = 0
    rate = { "rate": setRate }
    r = requests.post('http://localhost:8080/set-rate', json=rate)
    print "Set rate to: ", setRate

    page1 = requests.post('http://localhost:8080/transactions-hash')
    page2 = requests.post('http://localhost:8081/transactions-hash')

    if((ordered(page1.json())) != (ordered(page2.json()))):
        print "FAILED TO MATCH: "
        print jsonPrint(page1)
        print jsonPrint(page2)
    else:
        print "Successfully matched"


    #print jsonPrint(page1)
    print ">Number of transactions: ", page2.json()["numberOfTransactions"]
    print ">Transactions per second: ", page2.json()["numberOfTransactions"]/sleepTime
