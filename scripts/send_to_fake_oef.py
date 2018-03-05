import requests
import json
import pdb

def jsonPrint(r):
    return json.dumps(r.json(), indent=4, sort_keys=True)+"\n"

# Test Instance to register
instanceJSON = { "schema":
                   {
                     "name": "weather_data",
                     "attributes": [ { "name": "has_wind_speed", "type": "bool", "required": False }, { "name": "has_temperature", "type": "bool", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True } ],
                     "keywords": ["one", "two", "three"],
                     "description": "All possible weather data."
                   },
                 "values": [ { "has_wind_speed": "true" }, { "has_temperature": "true" }, { "latitude": "true" }, { "longitude": "true" } ],
                 "ID": "Joshyboy" }

# Register that instance
r = requests.post('http://localhost:8080/register-instance', json=instanceJSON)
print "Instance registration: ", jsonPrint(r)

# Query that instance, note keywords are there
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

r = requests.post('http://localhost:8080/query-instance', json=queryJSON)
print "Query: ", jsonPrint(r)

r = requests.post('http://localhost:8080/echo-query', json=queryJSON)
print "Query echo: ", jsonPrint(r)

exit(1)

r = requests.post('http://localhost:8080/check', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "Check user: ", r.json()

r = requests.post('http://localhost:8080/register', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "Register user: ", r.json()

r = requests.post('http://localhost:8080/register', json = {
    "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
print "Register user: ", r.json()

r = requests.post('http://localhost:8080/check', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "Check user: ", r.json()


for i in range(10):
    r = requests.post('http://localhost:8080/balance', json = {
        "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
    print "Get balance 830A0B9D-73EE-4001-A413-72CFCD8E91F3: ", r.json()

    r = requests.post('http://localhost:8080/balance', json = {
        "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
    print "Get balance 6164D5A6-A26E-43E4-BA96-A1A8787091A0: ", r.json()


    r = requests.post('http://localhost:8080/send', json = {
        "balance": 200,
        "fromAddress": "830A0B9D-73EE-4001-A413-72CFCD8E91F3",
        "notes": "hello world",
        "time": "1519650052994",
        "toAddress": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
    print "Send: ", r.json()


    r = requests.post('http://localhost:8080/balance', json = {
        "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
    print "Get balance 830A0B9D-73EE-4001-A413-72CFCD8E91F3: ", r.json()

    r = requests.post('http://localhost:8080/balance', json = {
        "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
    })
    print "Get balance 6164D5A6-A26E-43E4-BA96-A1A8787091A0: ", r.json()

r = requests.post('http://localhost:8080/get-transactions', json = {
    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
    })
print "History: ", r.text

