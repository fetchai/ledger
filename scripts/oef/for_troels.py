import requests
import json
import pdb

# print json in pretty way
def jsonPrint(r):
    return json.dumps(r.json(), indent=4, sort_keys=True)+"\n"

# Demo for Troels
base_lat = 51.5090446
base_lng = -0.0993713

# Test Instances (lazily constructed)
firstInstance = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ {"name" : "name", "type" : "string", "required" : True }, { "name": "latitude", "type": "float", "required": False }, { "name": "longitude", "type": "float", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True }, { "name": "price", "type": "int", "required": True } ],
                         "keywords": ["ignore"],
                         "description": "ignore"
                       },
                       "values": [ {"name" : "AEA_1"}, { "latitude": str(base_lat + 0.001) }, { "longitude": str(base_lng + 0.0021) }, {"price" : "100"}]},
                 "ID": "AEA_1" }

secondInstance = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ {"name" : "name", "type" : "string", "required" : True }, { "name": "latitude", "type": "float", "required": False }, { "name": "longitude", "type": "float", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True }, { "name": "price", "type": "int", "required": True } ],
                         "keywords": ["ignore"],
                         "description": "ignore"
                       },
                       "values": [ {"name" : "AEA_2"}, { "latitude": str(base_lat - 0.007) }, { "longitude": str(base_lng - 0.0034) }, {"price" : "50"}]},
                 "ID": "AEA_2" }

thirdInstance = { "instance" :
                    {"dataModel":
                       {
                         "name": "weather_data",
                         "attributes": [ {"name" : "name", "type" : "string", "required" : True }, { "name": "latitude", "type": "float", "required": False }, { "name": "longitude", "type": "float", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True }, { "name": "price", "type": "int", "required": True } ],
                         "keywords": ["ignore"],
                         "description": "ignore"
                       },
                       "values": [ {"name" : "AEA_3"}, { "latitude": str(base_lat + 0.002) }, { "longitude": str(base_lng + 0.0011) }, {"price" : "20"}]},
                 "ID": "AEA_3" }


# register these
r = requests.post('http://localhost:8080/register-instance', json=firstInstance)
r = requests.post('http://localhost:8081/register-instance', json=secondInstance)
r = requests.post('http://localhost:8082/register-instance', json=thirdInstance)

## Now Query (not multi-query)
#query = {
#        "constraints": [
#            {
#                "attribute": {
#                    "name": "latitude",
#                    "type": "float",
#                    "required": True
#                    },
#                "constraint": {
#                    "type": "relation",
#                    "op": ">=",
#                    "value_type": "float",
#                    "value": 101.1
#                    }
#            }],
#        "keywords" : [ "ignore"]
#        }


# this should work
query = {
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
                    "value": 200
                    }
            }],
        "keywords" : [ "one"]
        }

r = requests.post('http://localhost:8082/echo-query', json=query)
print "Query1 echo", jsonPrint(r)

r = requests.post('http://localhost:8082/query', json=query)
print "Query1 response", jsonPrint(r)

# Now do a multi-query
multiQuery = {
        "aeaQuery" : {
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
                        "value": 200
                        }
                }],
            "keywords" : [ "one"]
            },

        "forwardingQuery" : {
            "constraints": []
        }
    }

r = requests.post('http://localhost:8082/multi-query', json=multiQuery)
print "Query2 response", jsonPrint(r)

variable = raw_input('press key to deregister instances')

# deregister the instance
r = requests.post('http://localhost:8080/deregister-instance', json=firstInstance)
r = requests.post('http://localhost:8081/deregister-instance', json=secondInstance)
r = requests.post('http://localhost:8082/deregister-instance', json=thirdInstance)
