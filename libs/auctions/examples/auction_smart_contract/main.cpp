//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "mock_ledger.hpp"
#include "auction_smart_contract.hpp"

////#include "ledger/chaincode/dummy_contract.hpp"
//#include "auction_smart_contract/auction_smart_contract.hpp"
//#include <chrono>
//#include <random>







int main()
{

  n_blocks = 1000;

  // instantiate the mock ledger
  MockLedger mock_ledger();

  // instantiate transacting agents
  sellers
  bidders





  for (std::size_t j = 0; j < n_blocks; ++j)
  {

    mock_ledger.next_block();

    // agent transactions
    sellers.add_items
    bidders.add_bids

    //



  }


  // instantiate the contract
  BlockId start_block("start_block_id");
  BlockId end_block("end_block_id");
  VickreyAuctionContract va(start_block, end_block);

  // show listed nothings
  va.ShowListedItems();
  va.ShowBids();

  // add items
  std::vector<Item> all_items{Item(ItemId(0), AgentId(0), Value(0))};

  va.AddItem(all_items[0]);

  //
  va.PlaceBid(Bid(BidId(0), all_items, Value(0), AgentId(0)));
  //

  return 0 ;
}
