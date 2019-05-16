#!/usr/bin/env python3

# Quick script to hammer the example http server to induce a fail
import grequests

while True:
    dummy = input('Hit key to submit tx')
    urls = ['http://localhost:8080/pages' for x in range(1000)]

    # hammer the server with async http requests
    rs = (grequests.get(u) for u in urls)
    ret = grequests.map(rs)

    for x in ret:
        if(x):
            print("Found: " + x.text)
