#pragma once
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

#include "auctions/auction.hpp"

namespace fetch {
namespace auctions {

// template <typename A, typename V = fetch::ml::Variable<A>>
class VickreyAuction : public Auction
{
public:
  VickreyAuction(BlockId start_block_id, BlockId end_block_id)
    : Auction(start_block_id, end_block_id, false, std::numeric_limits<std::size_t>::max())
  {
    max_items_         = std::numeric_limits<std::size_t>::max();
    max_bids_          = std::numeric_limits<std::size_t>::max();
    max_items_per_bid_ = 1;
    max_bids_per_item_ = std::numeric_limits<std::size_t>::max();
  }

  bool Execute(BlockId current_block);

private:
  void SelectWinners();
};

}  // namespace auctions
}  // namespace fetch