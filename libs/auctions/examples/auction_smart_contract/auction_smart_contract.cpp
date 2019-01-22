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

//#include "ledger/chaincode/dummy_contract.hpp"
#include "auction_smart_contract/auction_smart_contract.hpp"
#include <chrono>
#include <random>

static constexpr std::size_t MINIMUM_TIME = 50;
static constexpr std::size_t MAXIMUM_TIME = 200;
static constexpr std::size_t DELTA_TIME   = MAXIMUM_TIME - MINIMUM_TIME;

using namespace fetch::auctions;


VickreyAuctionContract::VickreyAuctionContract(BlockId start_block, BlockId end_block) : ledger::Contract("fetch.dummy"), va(start_block, end_block)
{
  OnTransaction("wait", this, &VickreyAuctionContract::Wait);
  OnTransaction("run", this, &VickreyAuctionContract::Run);
}

VickreyAuctionContract::Status VickreyAuctionContract::Wait(Transaction const &)
{
  std::random_device rd;
  std::mt19937       rng;
  rng.seed(rd());

  // generate a random amount of 'work' time
  std::size_t const work_time = (rng() % DELTA_TIME) + MINIMUM_TIME;

  auto counter = counter_.load();

  FETCH_LOG_INFO(LOGGING_NAME, "Running complicated transaction ", counter, " ... ");

  // wait for the work time
  std::this_thread::sleep_for(std::chrono::milliseconds(work_time));

  FETCH_LOG_INFO(LOGGING_NAME, "Running complicated transaction ", counter, " ... complete");

  ++counter_;

  return Status::OK;
}

VickreyAuctionContract::Status VickreyAuctionContract::Run(Transaction const &)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Running that contract...");
  return Status::OK;
}

std::vector<Item> VickreyAuctionContract::ShowListedItems() const
{
  return VickreyAuctionContract::va.ShowListedItems();
}

std::vector<Bid> VickreyAuctionContract::ShowBids() const
{
  return VickreyAuctionContract::va.ShowBids();
};

ErrorCode VickreyAuctionContract::AddItem(Item const &item)
{
  return VickreyAuctionContract::va.AddItem(item);
};

//ErrorCode VickreyAuctionContract::PlaceBid(Bid bid)
//{
//  return VickreyAuctionContract::va.PlaceBid(bid);
//};

AgentId VickreyAuctionContract::Winner(ItemId item_id)
{
  return VickreyAuctionContract::va.Winner(item_id);
};


std::vector<AgentId> VickreyAuctionContract::Winners()
{
  return VickreyAuctionContract::va.Winners();
};

ItemContainer VickreyAuctionContract::items()
{
  return VickreyAuctionContract::va.items();
};






bool VickreyAuctionContract::BidIsValid()
{
//  asdfasdfads
  return true;
}

ErrorCode VickreyAuctionContract::PlaceBid(Bid bid)
{

  // check agent ID valid
  if (VickreyAuctionContract::BidIsValid(bid.bidder))
  {
    return VickreyAuctionContract::va.PlaceBid(bid);
  }
  return ErrorCode::invalid_bidder;


};
