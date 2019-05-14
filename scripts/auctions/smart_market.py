import fetch
import fetch.auctions
import fetch.byte_array


ba2 = fetch.byte_array.ByteArray("ExecuteBlockId")
ca = fetch.auctions.CombinatorialAuction(ba2)


item_id_offset = 200
seller_id_offset = 300
min_price_offset = 400
bid_id_offset = 500
bid_price_offset = 600
bid_bidder_offset = 700

# generate some items for sale
n_items = 10
items = []
count = 0
for cur_item in range(n_items):
    id = item_id_offset + count
    seller_id = seller_id_offset + count
    min_price = min_price_offset + count
    items.append(fetch.auctions.Item(id, seller_id, min_price))
    count += 1

# generate some bids on those items
n_bids = 100
bids = []
count = 0
for cur_bid in range(n_bids):
    id = bid_id_offset + count
    price = bid_price_offset + count
    bidder = bid_bidder_offset + count
    bids.append(fetch.auctions.Bid(id, [items[-1].Id()], price, bidder))
    count += 1

# add item
for cur_item in items:
    if (ca.AddItem(cur_item) == 0):
        print("add item success")

# sanity check - print item ids
li = ca.ShowListedItems()
for item in li:
    print(item.Id())

# add bid
for cur_bid in bids:
    if (ca.PlaceBid(cur_bid) == 0):
        print("place bid success")

# sanity check - print bid ids
bi = ca.ShowBids()
for bid in bi:
    print(bid.Id())

# mining
print("beginning mining")
run_length = 10
for cur_ran_seed in range(10):
    ca.Mine(cur_ran_seed, run_length)

# execute auction to determine winner
print("begin execution loop")

block_ids = ["wrong_block_id_1", "wrong_block_id_2",
             "wrong_block_id_3", "ExecuteBlockId", "wrong_block_id_4"]
for cur_block_id in block_ids:
    if (ca.Execute(fetch.byte_array.ByteArray(cur_block_id))):
        print("executed on block: " + cur_block_id)
    else:
        print("wrong block id: " + cur_block_id)
