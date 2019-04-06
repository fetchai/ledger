import matplotlib
import numpy as np
import copy
import random
import math









class SmartMarket(object):
    
    def __init__(self, mode='numerical_testing'):
        self.items = []
        self.bids = []
        self.name_to_id = {}
        self.normalisation = 1
        self.mode = mode

        if self.mode == 'numerical_testing':

            # numerical testing should print:
            # -597
            # 30
            # ---
            # -467

            self.active_random_count = 0
            self.nn_count = 0
            self.n_count = 0
            self.r_thresh_count = 0

            self.fixed_active_random = [1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1]
            self.fixed_nn = [2, 1, 1, 2, 1, 1, 2]
            self.fixed_n = [2, 4, 4, 6, 4, 5, 2, 6, 4, 0]
            self.fixed_r_thresh = [0.8916606598206824, 0.13927370519815263, 0.09483076347685593, 0.8102172359965896, 0.09876334465914771, 0.6839839319154413, 0.35379132951924075]
    
    def AddItem(self, name, min_price):
        self.name_to_id[name] = len(self.items)
        self.items.append(min_price)
        
        return self.name_to_id[name]
        
    def AddSingleBid(self, items, price):
        all_items = []
        for id in items:
            if isinstance(id, str):
                id = self.name_to_id[id]
            all_items.append(id)
            
        n = len(self.bids)
        self.bids.append({"items": all_items, "price": price, "excludes":[]})
        return n
    
    def AddBids(self, bids):
        excludes = []
        for b,p in bids:
            n = self.AddSingleBid(b,p)
            excludes.append(n)
        
        for i in excludes:
            self.bids[i]["excludes"] = copy.copy(excludes)

    def BuildGraph(self):
        # Building graph|
        self.local_fields = [ 0 for i in range(len(self.bids))]
        self.couplings = [ [0]*len(self.bids) for i in range(len(self.bids))]

        self.active = []
        for i in range(len(self.bids)):
            if self.mode == "numerical_testing":
                temp = self.fixed_active_random[self.active_random_count]
                self.active_random_count += 1
            else:
                temp = random.randint(0,1)
            self.active.append(temp)
            print("temp: " + str(temp))
        
        for i in range(len(self.bids)):
            bid1 = self.bids[i]
            
            e = bid1["price"]
            for n in range(len(self.items)):
                if n in bid1["items"]:
                    e -= self.items[n]
                
            print("e: " + str(e))
            self.local_fields[i] = e
            
            for j in range(i+1,len(self.bids)):
                bid2 = self.bids[j]
                
                coupling = 0
                if i in bid2["excludes"]:
                    coupling = 10
                    
                for n in range(len(self.items)):
                    if n in bid1["items"] and n in bid2["items"]:
                        coupling += (bid1["price"] + bid2["price"])
                
                self.couplings[i][j] = self.couplings[j][i] = -coupling
                print("self.couplings[i][j]: " + str(self.couplings[i][j]))

    def TotalBenefit(self):
        reward = 0
        for i in range(len(self.bids)):
            a1 = self.active[i]
            reward += a1*self.local_fields[i]
            
            for j in range(len(self.bids)):
                a2 = self.active[j]
                reward += a1*a2*self.couplings[i][j]
                
        return reward
    
    def SelectBid(self, bid):        
        for j in range(len(self.bids)):
            if self.couplings[bid][j] != 0:
                self.active[j] = 0
                
        self.active[bid] = 1
    
    def Mine(self, hash_value, runtime):
        random.seed(hash_value)
        self.BuildGraph()
        beta_start = 0.01
        beta_end = 1
        db = (beta_end-beta_start)/runtime
        beta = beta_start
        
        rejected = 0
        for s in range(runtime):
            for i in range(len(self.bids)):

                oldactive = copy.copy(self.active)
                oldreward = self.TotalBenefit()
                print("oldreward: " + str(oldreward))

                if self.mode == "numerical_testing":
                    nn = self.fixed_nn[self.nn_count]
                    self.nn_count += 1
                else:
                    nn = random.randint(1, 3)

                for gg in range(nn):

                    if self.mode == "numerical_testing":
                        n = self.fixed_n[self.n_count]
                        self.n_count += 1
                    else:
                        n = random.randint(0, len(self.bids)-1)

                    if self.active[n] == 1:
                        self.active[n] = 0
                    else:
                        self.active[n] = 1

                #self.SelectBid( n )
                newreward = self.TotalBenefit()
                de = oldreward - newreward
                print("oldreward: " + str(oldreward))
                print("newreward: " + str(newreward))
                print("de: " + str(de))

                if self.mode == "numerical_testing":
                    r_thresh = self.fixed_r_thresh[self.r_thresh_count]
                    self.r_thresh_count += 1
                else:
                    r_thresh = random.random()
                if r_thresh < math.exp(-beta*de):
                    pass
                    #print(oldreward, " => ", newreward, ":",de,beta,  math.exp(-beta*de))
                else:
                    self.active = oldactive
                    rejected += 1

            beta += db
        #print(rejected/ runtime/len(self.bids))

market = SmartMarket()

market.AddItem("car", 0)
market.AddItem("hotel", 0)
market.AddItem("cinema", 0)
market.AddItem("opera",0)

market.AddSingleBid(["car"], 10)
market.AddSingleBid(["hotel"],8)
market.AddSingleBid(["cinema"],5)
market.AddSingleBid([ "hotel", "car","cinema"], 29)
market.AddBids([ ([ "hotel", "car","cinema"], 20), ([ "hotel", "car","opera"], 30) ])
market.AddSingleBid(["opera"],7)

market.BuildGraph()
print(market.TotalBenefit())
market.SelectBid( 0 )
market.SelectBid( 1 )
market.SelectBid( 2 )
#market.SelectBid( 3 )
print(market.TotalBenefit())
print("---")
for i in range(1):
    market.Mine(23*i,1)
    print(market.TotalBenefit())
