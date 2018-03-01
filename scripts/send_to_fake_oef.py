import requests
#import json
#from json import dumps


# test breaks thingie
#test = { "thing" : "tester", "list" : [ {"one" : "me"}, {"two" : "asdf" } ] }
#r = requests.post('http://localhost:8080/register-instance', json=test)


#r = requests.post('http://localhost:8080/check', json = str({
#    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
#    }))
#
#print "Check user: ", r.json()

# Test Instance

instanceString = { "schema":
                   {
                     "name": "weather_data",
                     "values": "stand_in",
                     #"attributes": [ { "name": "wind_speed", "type": "bool", "required": "true" }, { "name": "temperature", "type": "bool", "required": "true" }, { "name": "air_pressure", "type": "bool", "required": "true" }, { "name": "humidity", "type": "bool", "required": "true" } ],
                     "description": "All possible weather data."
                   #},
                   #"values": [ { "humidity": "true" }, { "air_pressure": "true" }, { "temperature": "true" }, { "wind_speed": "true" } ] }
                   }}



r = requests.post('http://localhost:8080/register-instance', json=instanceString)
print "Instance registration: ", r.json()

#r = requests.post('http://localhost:8080/check', json = {
#    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
#    })
#print "Check user: ", r.json()
#
#r = requests.post('http://localhost:8080/register', json = {
#    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
#    })
#print "Register user: ", r.json()
#
#r = requests.post('http://localhost:8080/register', json = {
#    "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
#    })
#print "Register user: ", r.json()
#
#r = requests.post('http://localhost:8080/check', json = {
#    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
#    })
#print "Check user: ", r.json()
#
#
#for i in range(10):
#    r = requests.post('http://localhost:8080/balance', json = {
#        "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
#    })
#    print "Get balance 830A0B9D-73EE-4001-A413-72CFCD8E91F3: ", r.json()
#
#    r = requests.post('http://localhost:8080/balance', json = {
#        "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
#    })
#    print "Get balance 6164D5A6-A26E-43E4-BA96-A1A8787091A0: ", r.json()
#
#
#    r = requests.post('http://localhost:8080/send', json = {
#        "balance": 200,
#        "fromAddress": "830A0B9D-73EE-4001-A413-72CFCD8E91F3",
#        "notes": "hello world",
#        "time": "1519650052994",
#        "toAddress": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
#    })
#    print "Send: ", r.json()
#
#
#    r = requests.post('http://localhost:8080/balance', json = {
#        "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
#    })
#    print "Get balance 830A0B9D-73EE-4001-A413-72CFCD8E91F3: ", r.json()
#
#    r = requests.post('http://localhost:8080/balance', json = {
#        "address": "6164D5A6-A26E-43E4-BA96-A1A8787091A0"
#    })
#    print "Get balance 6164D5A6-A26E-43E4-BA96-A1A8787091A0: ", r.json()
#
#r = requests.post('http://localhost:8080/get-transactions', json = {
#    "address": "830A0B9D-73EE-4001-A413-72CFCD8E91F3"
#    })
#print "History: ", r.text



#thingie = {
#  "firstName": "John",
#  "lastName": "Smith",
#  "isAlive": True,
#  "age": 27,
#  "address": {
#    "streetAddress": "21 2nd Street",
#    "city": "New York",
#    "state": "NY",
#    "postalCode": "10021-3100"
#  },
#  "phoneNumbers": [
#    {
#      "type": "home",
#      "number": "212 555-1234"
#    },
#    {
#      "type": "office",
#      "number": "646 555-4567"
#    },
#    {
#      "type": "mobile",
#      "number": "123 456-7890"
#    }
#  ],
#  "children": []
#}
