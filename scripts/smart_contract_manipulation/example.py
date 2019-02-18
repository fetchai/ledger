#!/usr/bin/env python3

import requests
import time
import json
import pdb
import random
import optparse
import threading
import sys, os

# print json in pretty way
def jsonPrint(r):
    try:
        return json.dumps(r.json(), indent=4, sort_keys=True)+"\n"
    except:
        return "FAILED TO PARSE JSON"

# Order map for comparison
#https://stackoverflow.com/questions/25851183/
def ordered(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered(x) for x in obj)
    else:
        return obj

# Make HTTP post
def HTTPpost(endpoint, page, jsonArg="{}"):
    return requests.post('http://'+str(endpoint["IP"])+':'+str(endpoint["HTTPPort"])+'/'+page, json=jsonArg)

threads = []
def HTTPpostAsync(endpoint, page, jsonArg="{}"):
        thread = threading.Thread(target=HTTPpost, args=(endpoint, page, jsonArg))
        thread.start()
        threads.append(thread)

# Will act as controlling node if used
endpoint  = {"HTTPPort": 8000, "TCPPort": 9080, "IP": "localhost"}


#arg = { "smart_contract" : "function main() Print('Hello, world');  endfunction" }

#arg = { "smart_contract" : "function main() \nPrint('Hello, world'); \nvar s = State<Int32>('hello');\n Print(toString(s));\n s.set(12.34);\n Print(toString(s)); endfunction " }

arg = { "smart_contract" : "function main() var a = 2; var b : Int32 = 1; b = a + b; Print('The result is: ' + toString(b)); endfunction" }

resp = HTTPpost(endpoint, 'api/wallet/create_smart_contract', arg)

SC_HASH = ""

print(resp.text)
print(jsonPrint(resp))

try:
    #import ipdb; ipdb.set_trace(context=20)
    SC_HASH = resp.json()["SC_HASH"]
except:
    print("Failed to find sc hash")
    exit(1)

time.sleep(5)
#dummy = input("here: ")

# Now invoke the SC!
arg = { "smart_contract" : SC_HASH, "UUID" : SC_HASH + ".PUBKEY.main" }

resp = HTTPpost(endpoint, 'api/wallet/invoke_smart_contract', arg)

SC_HASH = ""

print(resp.text)
print(jsonPrint(resp))
