import requests
import json
import pdb

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

#pdb.set_trace()

# TODO: (`HUT`) : this in a nice way
page="debug-all-nodes"
print "testing page ", page
r = requests.post('http://localhost:8080/'+page)

for i in range(10):
    #r2 = requests.post('http://localhost:'+str(8080+i)+'/debug-all-nodes')
    try:
        r2 = requests.post('http://localhost:'+str(8080+i)+'/'+page, json='{}')
    except:
        print "Failed to access http page", str(8080+i)
        break

    if(ordered(r.json()) != ordered(r2.json())):
        print "FAIL TO MATCH: ", i
        print jsonPrint(r)
        print jsonPrint(r2)
        exit(1)

    print "Matched page: ", str(8080+i)

instanceJSON = { "max_number" : 10 }
page="debug-all-events"
print "testing page ", page
r = requests.post('http://localhost:8080/'+page, json=instanceJSON)

for i in range(10):
    #r2 = requests.post('http://localhost:'+str(8080+i)+'/debug-all-nodes')
    try:
        r2 = requests.post('http://localhost:'+str(8080+i)+'/'+page, json=instanceJSON)
    except:
        print "Failed to access http page", str(8080+i)
        break

    if(ordered(r.json()) != ordered(r2.json())):
        print "FAIL TO MATCH: ", i
        print jsonPrint(r)
        print jsonPrint(r2)
        exit(1)

    print "Matched page: ", str(8080+i)
