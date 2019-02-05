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

namespace fetch {
namespace auctions {

enum class ErrorCode
{
  SUCCESS,
  PREVIOUS_AUCTION_NOT_CLEARED,
  ITEM_ALREADY_LISTED,
  AUCTION_FULL,
  ITEM_NOT_LISTED,
  TOO_MANY_BIDS,
  AUCTION_CLOSED,
  AUCTION_STILL_LISTING,
  TOO_MANY_ITEMS,
  ITEM_ID_ERROR,
  AGENT_ID_ERROR,
  ITEM_MIN_PRICE_ERROR,
  INVALID_BID_ID,
  REPEAT_BID_ID,
  BID_PRICE,
  BID_BIDDER_ID,
  INCORRECT_END_BLOCK
};

}  // namespace auctions
}  // namespace fetch