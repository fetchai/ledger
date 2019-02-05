import requests

##############
# list items #
##############


all_items = []
all_items.append(
{
    "item_id" : 0,
    "seller_id" : 0,
    "min_price" : 5.0
}
)
all_items.append(
{
    "item_id" : 1,
    "seller_id" : 1,
    "min_price" : 10.0
}
)
all_items.append(
{
    "item_id" : 2,
    "seller_id" : 1,
    "min_price" : 15.0
}
)

for cur_item in all_items:
    r = requests.post('http://127.0.0.1:8080/api/item/list', json=cur_item)
    assert(r.status_code == 200)
print("items listed successfully")

#############
# list bids #
#############

all_bids = []
all_bids.append(
{
    "bid_id" : 0,
    "item_ids" : [0, 1],
    "bid_price": 17.0,
    "bidder_id": 0,
    "excludes": [2]
}
)
all_bids.append(
{
    "bid_id" : 1,
    "item_ids" : [0, 1],
    "bid_price": 19.0,
    "bidder_id": 1,
    "excludes": [3]
}
)
all_bids.append(
{
    "bid_id" : 2,
    "item_ids" : [1, 2],
    "bid_price": 30.0,
    "bidder_id": 0,
    "excludes": [0]
}
)
all_bids.append(
{
    "bid_id" : 3,
    "item_ids" : [1, 2],
    "bid_price": 32.0,
    "bidder_id": 1,
    "excludes": [1]
}
)
all_bids.append(
{
    "bid_id" : 4,
    "item_ids" : [2],
    "bid_price": 5.0,
    "bidder_id": 2
}
)


for cur_bid in all_bids:
    r = requests.post('http://127.0.0.1:8080/api/bid/place', json=cur_bid)
    assert(r.status_code == 200)
print("bids placed succesfully")


########
# mine #
########

mine_params = {
    "random_seed" : 123456,
    "run_time" : 1000
}
r = requests.post('http://127.0.0.1:8080/api/mine', json=mine_params)
assert(r.status_code == 200)
print("mining successful")


##########
# result #
##########

# get result
# mine_params = {
#     "random_seed" : 0,
#     "run_time" : 100
# }
r = requests.post('http://127.0.0.1:8080/api/execute', json={})
assert(r.status_code == 200)
print("execution successful")

