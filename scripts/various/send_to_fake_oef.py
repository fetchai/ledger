import requests


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
