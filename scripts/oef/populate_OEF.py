import requests
import time
import json
import pdb
import random

#import random as rand
import sys, os

#rand.seed(42)
#
#home = os.path.expanduser('~')
#sys.path.insert(0, home + '/repos/bayeux/')
#from p2psims import *

#exit(1)

# print json in pretty way
def jsonPrint(r):
    return json.dumps(r.json(), indent=4, sort_keys=True)+"\n"

# Demo for Troels
base_lat = 51.5090446 + 0.01
base_lng = -0.0993713 + 0.01

51.5090446 -0.0993713

# list of preset node locations (not sorted)
sortedNodeLocations = [
		       [51.4944201,-0.1023585],  # eleph and castle
		       [51.515133, -0.0999188],  # old bailey
                       [51.5045303, -0.0994679], # ewer st
                       [51.5145303, -0.0924679], # gresham st
                       [51.506993,-0.1142297],   # royal national theatre
                       [51.5009331,-0.1064792],  # webber street
                       [51.5091228,-0.0851453],  # lower thames street
                       [51.5138971,-0.1272004],  # shelto st
                       [51.4769225,-0.1290553],  # lambeth place
                       [51.5017079,-0.1314588],  # bermondsay
                       [51.5194996,-0.0633604],  # whitechapel station
                       [51.4843681,-0.0868746],  # albany road
                       [51.4837512,-0.1170999],  # kia oval
                       [51.4841678,-0.1087366],  # eleph, and castle
                       [51.4920728,-0.1295575],  # page st
                       [51.5004525,-0.0787364],  # druid street
                       [51.4967159,-0.0922306],  # grange road
                       [51.4875868,-0.0948443],  # data st
                       [51.5164725,-0.1183714]]  # corams fields

## Just in case, sort
#sortedNodeLocations = sorted(nodeLocations, key=lambda nodeLocation: nodeLocation[1])
#print sortedNodeLocations  # sort by longitude, which means nodes will be assigned left to right

workingNodes = 0

# Setup the nodes in these locations
page="set-node-latlong"
for i in range(len(sortedNodeLocations)-1):
    try:
        #variable = raw_input('press key to registe node: '+str(8080+i))
        print "latitude", sortedNodeLocations[i][0], "longitude", sortedNodeLocations[i][1]
	#variable = raw_input('press key to set node')
        r2 = requests.post('http://localhost:'+str(8080+i)+'/'+page, json={ "latitude" : sortedNodeLocations[i][0], "longitude" : sortedNodeLocations[i][1] })
    except:
        print "Failed to set node at http page", str(8080+i)
        break

for i in range(10000):
    try:
        requests.post('http://localhost:'+str(8080+i)+'/debug-all-nodes', json='{}')
        workingNodes = workingNodes + 1
    except:
        print "Failed to access http page", str(8080+i)
        print "Going on the assumption there are ", workingNodes, " workingNodes"
        break

AEAs_per_node              = 10;
AEAs_latlong_distance_from = 0.001;

# Add endpoints to Nodes that have poor connectivity
connection = {"TCPPort": 9082, "IP": "localhost"}
requests.post('http://localhost:8081/add-connection', json=connection)

# generate an/some events by selecting a random aea
for i in range(1, workingNodes-1):
    connection = {"TCPPort": 9080+i, "IP": "localhost"}
    requests.post('http://localhost:8080/add-connection', json=connection)

variable = raw_input('press key to registe AEAs')

# Template AEA
templateInstance = { "instance" :
                    {"dataModel":
                       {
                         "name": "template_datamodel",
                         "attributes": [ {"name" : "name", "type" : "string", "required" : True }, { "name": "latitude", "type": "float", "required": False }, { "name": "longitude", "type": "float", "required": True }, { "name": "latitude", "type": "bool", "required": True }, { "name": "longitude", "type": "bool", "required": True }, { "name": "price", "type": "int", "required": True } ],
                         "keywords": ["ignore"],
                         "description": "ignore"
                       },
                       "values": [ {"name" : "AEA_1"}, { "latitude": str(base_lat + 0.001) }, { "longitude": str(base_lng + 0.0021) }, {"price" : "100"}]},
                 "ID": "AEA_1" }

aeaNames = []

for node in range(workingNodes):
    for i in range(AEAs_per_node-1):
        aea = templateInstance
        shiftLat  = random.uniform(0.001, 0.003) * (1 - (random.randint(0,1)*2))
        shiftLong = random.uniform(0.001, 0.003) * (1 - (random.randint(0,1)*2))
        price     = random.randint(0, 100)

        name = "AEA_"+str(8080+node)+"_"+str(i)
        host = str(8080+node)
        aeaNames.append([name, host])

        setLat = str(sortedNodeLocations[node][0] + shiftLat);
        setLng = str(sortedNodeLocations[node][1] + shiftLong);

        aea["instance"]["values"] = [ {"name" : name}, { "latitude": setLat }, { "longitude": setLng }, {"price" : str(price)}]
        requests.post('http://localhost:'+host+'/register-instance', json=aea)

        time.sleep(0.01)

#pdb.set_trace()

for i in range(10):

    variable = raw_input('press key to submit a mult query')

    # Now do a multi-query, as if it came from one of the AEAs (do three of these)
    randomSelect = random.randint(0,len(aeaNames)-1)
    #randomAEA = aeaNames[randomSelect][0]
    #randomHost = aeaNames[randomSelect][1]

    randomAEA = "AEA_"+str(8080+workingNodes-1)+"_1"
    randomHost = str(8080+workingNodes-1)

    print "Random AEA: ", randomAEA
    print "Random host: ", randomHost

    multiQuery = {
            "ID": randomAEA,
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
                            "value": 10
                            }
                    }],
                "keywords" : [ "ignore"]
                },

            "forwardingQuery" : {
                "constraints": [
                    {
                        "attribute": {
                            "name": "latitude",
                            "type": "float",
                            "required": True
                            },
                        "constraint": {
                            "type": "relation",
                            "op": "<=",
                            "value_type": "float",
                            "value": float(510.51035)  # want south of the river:  51.5103576,-0.109238
                            }
                    }],
                "keywords" : [ "ignore"]
            }
        }

    try:
        r = requests.post('http://localhost:'+randomHost+'/multi-query', json=multiQuery)
        print "Mult query response", jsonPrint(r)
    except:
        print "Tried and failed to query: ", randomHost
