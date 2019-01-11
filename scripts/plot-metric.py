#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys
import pdb

#mu, sigma = 100, 15
#x = mu + sigma * np.random.randn(10000)
#hist, bins = np.histogram(x, bins=50)
#width = 0.7 * (bins[1] - bins[0])
#center = (bins[:-1] + bins[1:]) / 2
#plt.bar(center, hist, align='center', width=width)
#plt.show()

file = open("./build/metrics.csv")

transactions = {}

for line in file:
    #print(line)
    if 'transaction' in line:
        #transactions[line
        split_on_comma = line.split(',')
        #print(split_on_comma)
        if split_on_comma[3] not in transactions:
            transactions[split_on_comma[3]] = {split_on_comma[2] : split_on_comma[0]}
        else:
            transactions[split_on_comma[3]][split_on_comma[2]] = split_on_comma[0]
        #print(transactions)
        #sys.exit(1)

#print(transactions)

times = []


for transaction in transactions:
    try:
        start_time  = int(transactions[transaction]["submitted"])
        finish_time = int(transactions[transaction]["stored"])

        total_time = (finish_time - start_time)/1000000
        #print(total_time)
        #sys.exit(1)
        times.append(total_time)
    except:
        pass


times.sort()

upper_percentile = len(times) // 100

target = times[-upper_percentile]
for i in range(len(times)-upper_percentile, len(times), 1):
    times[i] = target

#times = times[:-50]

hist, bins = np.histogram(times, bins=100)
width = 0.7 * (bins[1] - bins[0])
center = (bins[:-1] + bins[1:]) / 2
plt.bar(center, hist, align='center', width=width)
plt.show()

#mu, sigma = 100, 15
#x = mu + sigma * np.random.randn(10000)
#pdb.set_trace()

#hist, bins = np.histogram(x, bins=50)
#width = 0.7 * (bins[1] - bins[0])
#center = (bins[:-1] + bins[1:]) / 2
#plt.bar(center, hist, align='center', width=width)
#plt.show()
