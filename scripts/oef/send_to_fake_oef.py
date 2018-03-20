import requests
import json
import pdb

# print json in pretty way
def jsonPrint(r):
    return json.dumps(r.json(), indent=4, sort_keys=True)+"\n"

# Price query demo for Josh

# Test Instances to register (all price 99)
instanceJSON = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ {"name" : "price", "type" : "int", "required" : True }, { "name": "has_wind_speed", "type": "bool", "required": False }, { "name": "has_temperature", "type": "bool", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True } ],
                         "keywords": ["one", "two", "three"],
                         "description": "All possible weather data."
                       },
                       "values": [ {"price" : "99"}, { "has_wind_speed": "true" }, { "has_temperature": "true" }, { "latitude": "true" }, { "longitude": "true" } ]},
                 "ID": "Josh" }

secondInstanceJSON = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ {"name" : "price", "type" : "int", "required" : True }, { "name": "has_wind_speed", "type": "bool", "required": False }, { "name": "has_temperature", "type": "bool", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True } ],
                         "keywords": ["one", "two", "three"],
                         "description": "All possible weather data."
                       },
                       "values": [ {"price" : "99"}, { "has_wind_speed": "true" }, { "has_temperature": "true" }, { "latitude": "true" }, { "longitude": "true" } ]},
                 "ID": "Nathan" }

thirdInstJSON = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ {"name" : "price", "type" : "int", "required" : True }, { "name": "has_wind_speed", "type": "bool", "required": False }, { "name": "has_temperature", "type": "bool", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True } ],
                         "keywords": ["one", "two", "three"],
                         "description": "All possible weather data."
                       },
                       "values": [ {"price" : "99"}, { "has_wind_speed": "true" }, { "has_temperature": "true" }, { "latitude": "false" }, { "longitude": "false" } ]},
                 "ID": "Jon" }


# register these
r = requests.post('http://localhost:8080/register-instance', json=thirdInstJSON)
r = requests.post('http://localhost:8080/register-instance', json=secondInstanceJSON)
r = requests.post('http://localhost:8080/register-instance', json=instanceJSON)

# Now Query against the price
queryJSON_success = {
        "constraints": [
            {
                "attribute": {
                    "name": "price",
                    "type": "int",
                    "required": True
                    },
                "constraint": {
                    "type": "relation",
                    "op": "<=",
                    "value_type": "int",
                    "value": 99
                    }
            }],
        "keywords" : [ "one"]
        }


queryJSON_fail = {
        "constraints": [
            {
                "attribute": {
                    "name": "price",
                    "type": "int",
                    "required": True
                    },
                "constraint": {
                    "type": "relation",
                    "op": ">",
                    "value_type": "int",
                    "value": 101
                    }
            }],
        "keywords" : [ "one"]
        }

r = requests.post('http://localhost:8080/query-for-agents-instances', json=queryJSON_success)
print "price query1", r
print "price query1", jsonPrint(r)

r = requests.post('http://localhost:8080/query-for-agents-instances', json=queryJSON_fail)
print "price query2", jsonPrint(r)

r = requests.post('http://localhost:8080/debug-all-nodes')
print "test this", jsonPrint(r)
exit(1)

# Test Instance to register
instanceJSON = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ { "name": "has_wind_speed", "type": "bool", "required": False }, { "name": "has_temperature", "type": "bool", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True } ],
                         "keywords": ["one", "two", "three"],
                         "description": "All possible weather data."
                       },
                     "values": [ { "has_wind_speed": "true" }, { "has_temperature": "true" }, { "latitude": "true" }, { "longitude": "true" } ]},
                 "ID": "test_user" }

# Register that instance
r = requests.post('http://localhost:8080/register-instance', json=instanceJSON)
print "Instance registration: ", jsonPrint(r)

# Query that instance, note keywords are there (must match all)
queryJSON = {
        "constraints": [
            {
                "attribute": {
                    "name": "has_wind_speed",
                    "type": "bool",
                    "required": True
                    },
                "constraint": {
                    "type": "relation",
                    "op": "=",
                    "value_type": "bool",
                    "value": True
                    }
            },
            {
                "attribute": {
                    "name": "has_temperature",
                    "type": "bool",
                    "required": True
                    },
                "constraint": {
                    "type": "relation",
                    "op": "=",
                    "value_type": "bool",
                    "value": True
                    }
            },
            {
                "attribute": {
                    "name": "has_temperature",
                    "type": "bool",
                    "required": True
                    },
                "constraint": {
                    "type": "relation",
                    "op": "=",
                    "value_type": "bool",
                    "value": True
                    }
            }],
        "keywords" : [ "two", "one"]
        }

# Try buying from an AEA (should be using an active one)
req = { "ID" : "listening_aea" }

r = requests.post('http://localhost:8080/buy-from-aea', json=req)
print "buy aea result: ", jsonPrint(r)

# Test substring matching functionality
instanceJSONkeyw = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ { "name": "has_wind_speed", "type": "bool", "required": False }, { "name": "unique", "type": "bool", "required": True } ],
                         "keywords": ["one long string testyyy"],
                         "description": "All possible weather data."
                       },
                     "values": [ { "has_wind_speed": "true" }, {"unique" : "true"}  ]},
                 "ID": "substr_user_withyyy" }

instanceJSONattr = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ { "name": "has_wind_speedxxx", "type": "bool", "required": False }, { "name": "unique", "type": "bool", "required": True }],
                         "keywords": ["one long string test"],
                         "description": "All possible weather data."
                       },
                       "values": [ { "has_wind_speedxxx": "true" }, {"unique" : "true"} ]},
                 "ID": "substr_user_withxxx" }

queryJSONkeyw = {
        "constraints": [
            {
                "attribute": {
                    "name": "unique",
                    "type": "bool",
                    "required": True
                    },
                "constraint": {
                    "type": "relation",
                    "op": "=",
                    "value_type": "bool",
                    "value": True
                    }
            }],
        "keywords" : [ "yyy"]
        }

queryJSONattr = {
        "constraints": [
            {
                "attribute": {
                    "name": "unique",
                    "type": "bool",
                    "required": True
                    },
                "constraint": {
                    "type": "relation",
                    "op": "=",
                    "value_type": "bool",
                    "value": True
                    }
            }],
        "keywords" : [ "xxx"]
        }


r = requests.post('http://localhost:8080/register-instance', json=instanceJSONkeyw)
r = requests.post('http://localhost:8080/register-instance', json=instanceJSONattr)

r = requests.post('http://localhost:8080/query-for-agents-instances', json=queryJSONkeyw)
print "query intending to hit keyword: ", jsonPrint(r)

r = requests.post('http://localhost:8080/query-for-agents-instances', json=queryJSONattr)
print "query intending to hit attribute: ", jsonPrint(r)

exit(1)

# Ledger transactions
r = requests.post('http://localhost:8080/check', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "Check user (expect false): ", jsonPrint(r)

r = requests.post('http://localhost:8080/register', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "Register user: ", jsonPrint(r)

r = requests.post('http://localhost:8080/register', json = {
    "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
print "Register user: ", jsonPrint(r)

r = requests.post('http://localhost:8080/check', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "Check user (expect true): ", jsonPrint(r)


for i in range(10):
    r = requests.post('http://localhost:8080/balance', json = {
        "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
    print "Get balance 830A0B9D-73EE-4001-A413-72CFCD8E91F3: ", jsonPrint(r)

    r = requests.post('http://localhost:8080/balance', json = {
        "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
    print "Get balance 6164D5A6-A26E-43E4-BA96-A1A8787091A0: ", jsonPrint(r)


    r = requests.post('http://localhost:8080/send', json = {
        "balance": 200,
        "fromAddress": "830A0B9D-73EE-4001-A413-72CFCD8E91F3",
        "notes": "hello world",
        "time": "1519650052994",
        "toAddress": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
    print "Send: ", jsonPrint(r)


    r = requests.post('http://localhost:8080/balance', json = {
        "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
    print "Get balance 830A0B9D-73EE-4001-A413-72CFCD8E91F3: ", jsonPrint(r)

    r = requests.post('http://localhost:8080/balance', json = {
        "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
    print "Get balance 6164D5A6-A26E-43E4-BA96-A1A8787091A0: ", jsonPrint(r)

r = requests.post('http://localhost:8080/get-transactions', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "History: ", r.text # note, this appears to be incorrectly formatted; can't be parsed to json

