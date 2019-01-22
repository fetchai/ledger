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

#include "agent.hpp"
#include "auction_smart_contract.hpp"
#include "auctions/type_def.hpp"
#include "mock_ledger.hpp"

////#include "ledger/chaincode/dummy_contract.hpp"
//#include "auction_smart_contract/auction_smart_contract.hpp"
//#include <chrono>
//#include <random>

using namespace fetch::auctions;
using namespace fetch::auctions::example;

struct SimParams
{
  std::size_t n_blocks           = 1000;
  std::size_t n_sellers          = 2;
  std::size_t n_buyers           = 4;
  std::size_t n_items_per_seller = 2;
  std::size_t n_bids_per_buyer   = 1;
};

void GenerateSellers(SimParams &params, std::size_t &item_counter, std::size_t &agent_counter,
                     std::vector<Agent> &ret)
{
  for (std::size_t j = 0; j < params.n_sellers; ++j)
  {
    // generate some new items for sale
    std::vector<Item> items{};
    for (std::size_t k = 0; k < params.n_items_per_seller; ++k)
    {
      // register new items with min price set to 0
      Item cur_item(ItemId(item_counter), AgentId(agent_counter), Value(0));
      items.emplace_back(cur_item);
      item_counter++;
    }

    // add new agent
    ret.emplace_back(Agent(j, items));
    agent_counter++;
  }
}

void GenerateBuyers(SimParams &params, std::size_t &bid_counter, std::size_t &agent_counter,
                    std::vector<Agent> &ret)
{
  for (std::size_t j = 0; j < params.n_buyers; ++j)
  {
    // generate some new items for sale
    std::vector<Bid> bids{};
    for (std::size_t k = 0; k < params.n_bids_per_buyer; ++k)
    {
      // register new bids with value = j
      bids.emplace_back(Bid(BidId(bid_counter), std::vector<ItemId>{0},  // only try to buy item 0
                            Value(j), AgentId(agent_counter), std::vector<Bid>{})  // excluded bids
      );
      bid_counter++;
    }

    // add new agent
    ret.emplace_back(Agent(j, bids));
    agent_counter++;
  }
}

int main()
{
  SimParams   params;
  std::size_t item_counter  = 0;
  std::size_t agent_counter = 0;
  std::size_t bid_counter   = 0;

  // instantiate mock ledger
  MockLedger ml{};

  // instantiate the participating sellers
  std::vector<Agent> sellers{};
  GenerateSellers(params, item_counter, agent_counter, sellers);

  // instantiate the participating buyers
  std::vector<Agent> buyers{};
  GenerateBuyers(params, bid_counter, agent_counter, buyers);

  // instantiate the smart contract once & instantiate the mock ledger
  Agent contract_owner{agent_counter};
  agent_counter++;
  BlockId                end_block = BlockId(params.n_blocks);
  VickreyAuctionContract smart_contract(contract_owner.id(), end_block,
                                        std::make_shared<MockLedger>(ml));

  std::cout << "bid_counter: " << bid_counter << std::endl;
  std::cout << "item_counter: " << item_counter << std::endl;
  std::cout << "agent_counter: " << agent_counter << std::endl;

  // run simulation
  for (std::size_t j = 0; j < params.n_blocks; ++j)
  {
    // sellers attempts to list some items (only works if auction in the correct phase)
    for (std::size_t k = 0; k < sellers.size(); ++k)
    {
      if (!sellers[k].items_listed)
      {
        smart_contract.Call(sellers[k].id(), Mode::ADD_ITEMS, sellers[k].items());
        sellers[k].items_listed = true;
      }
    }

    // buyers make bids, reveal bids, or collect winnings depending on phase
    for (std::size_t k = 0; k < buyers.size(); ++k)
    {
      if (!buyers[k].bids_placed)
      {
        smart_contract.Call(buyers[k].id(), Mode::PLACE_BIDS, {},  // no items to list
                            buyers[k].bids());
        buyers[k].bids_placed = true;
      }
      else if (!buyers[k].bids_revealed)
      {
        smart_contract.Call(buyers[k].id(), Mode::REVEAL_BIDS, {},  // no items to list
                            buyers[k].bids());
        buyers[k].bids_revealed = true;
      }
      else if (!buyers[k].wins_collected)
      {
        smart_contract.Call(buyers[k].id(), Mode::COLLECT_WINNINGS, {},  // no items to list
                            buyers[k].bids(), buyers[k].funds);
        buyers[k].bids_revealed = true;
      }
    }

    // transactions packed into next block
    ml.AddNewBlock();
  }

  return 0;
}
