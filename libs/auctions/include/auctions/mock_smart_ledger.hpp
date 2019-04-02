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

#include "core/assert.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "variant/variant_utils.hpp"

#include "auctions/combinatorial_auction.hpp"
#include "auctions/type_def.hpp"
#include "math/tensor.hpp"

namespace fetch {
namespace auctions {

/**
 * class offering a HTTP interface to a smart market (i.e. combinatorial auction). Ledger
 * integration details ignored or mocked as necessary for now.
 */
class MockSmartLedger : public fetch::http::HTTPModule
{
public:
  static constexpr char const *LOGGING_NAME = "MockSmartLedger";

  enum class ErrorCode
  {
    NOT_IMPLEMENTED = 1000,
    PARSE_FAILURE
  };

  MockSmartLedger()
  {
    auction_ = CombinatorialAuction();

    // TODO(private 597): implement timer & cycling auction clearance

    /////////////////////////////////
    /// Register valid http calls ///
    /////////////////////////////////

    Post("/api/item/list", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnListItem(request);
    });
    Post("/api/bid/place", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnPlaceBid(request);
    });
    Post("/api/mine", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnMine(request);
    });
    Post("/api/execute", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnExecute(request);
    });
  }

  ////////////////////////
  /// C++ method calls ///
  ////////////////////////

  ItemContainer items()
  {
    return auction_.items();
  }

  std::vector<Bid> bids()
  {
    return auction_.ShowBids();
  }

  void Mine()
  {
    auction_.Mine(1234, 10);
  }

  void Execute()
  {
    auction_.Execute();

    for (std::size_t j = 0; j < auction_.ShowBids().size(); ++j)
    {
      std::cout << "j: " << j << ", status: " << auction_.Active(j) << std::endl;
    }
  }

  void Reset()
  {
    auction_.Reset();
  }

  void ShowAuctionResult()
  {
    auction_.ShowAuctionResult();
  }

private:
  CombinatorialAuction auction_;

  /**
   * method for listing a new item in the auction
   */
  http::HTTPResponse OnListItem(http::HTTPRequest const &request)
  {
    try
    {
      // parse the json request
      json::JSONDocument doc;
      doc.Parse(request.body());

      ItemId  item_id   = DEFAULT_ITEM_ID;
      AgentId seller_id = DEFAULT_ITEM_AGENT_ID;
      Value   min_price = DEFAULT_ITEM_MIN_PRICE;

      // extract all the request parameters
      if (variant::Extract(doc.root(), "item_id", item_id) &&
          variant::Extract(doc.root(), "seller_id", seller_id) &&
          variant::Extract(doc.root(), "min_price", min_price))
      {
        fetch::auctions::ErrorCode ec = auction_.AddItem(Item{item_id, seller_id, min_price});

        if (ec == fetch::auctions::ErrorCode::SUCCESS)
        {
          std::cout << "item: " << item_id << ", listed for seller_id: " << seller_id
                    << ", at min price: " << min_price << std::endl;
        }
        return http::CreateJsonResponse(R"({"success": true})", http::Status::SUCCESS_OK);
      }
    }
    catch (json::JSONParseException const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input transfer request: ", ex.what());
    }

    return BadJsonResponse(ErrorCode::PARSE_FAILURE);
  }

  /**
   * method for placing new bids
   */
  http::HTTPResponse OnPlaceBid(http::HTTPRequest const &request)
  {
    try
    {
      // parse the json request
      json::JSONDocument doc;
      doc.Parse(request.body());

      BidId               bid_id      = DEFAULT_BID_ID;
      std::vector<ItemId> item_ids    = {};
      Value               bid_price   = DEFAULT_BID_PRICE;
      AgentId             bidder_id   = DEFAULT_BID_BIDDER;
      std::vector<BidId>  excludes    = {};
      bool                exclude_all = false;

      // extract all the request parameters
      if (variant::Extract(doc.root(), "bid_id", bid_id) &&
          variant::Extract(doc.root(), "item_ids", item_ids) &&
          variant::Extract(doc.root(), "bid_price", bid_price) &&
          variant::Extract(doc.root(), "bidder_id", bidder_id))
      {

        variant::Extract(doc.root(), "excludes", excludes);
        variant::Extract(doc.root(), "exclude_all", exclude_all);

        fetch::auctions::ErrorCode ec;
        if (exclude_all == false)
        {
          ec = auction_.PlaceBid(Bid{bid_id, item_ids, bid_price, bidder_id, excludes});
        }
        else
        {
          ec = auction_.PlaceBid(Bid{bid_id, item_ids, bid_price, bidder_id, exclude_all});
        }

        if (ec == fetch::auctions::ErrorCode::SUCCESS)
        {
          std::cout << "bidder: " << bidder_id << " placed bid of value: " << bid_price
                    << ", on item_ids: ";
          for (std::size_t i = 0; i < item_ids.size(); ++i)
          {
            std::cout << item_ids[i] << ", ";
          }
          std::cout << std::endl;
        }
        return http::CreateJsonResponse(R"({"success": true})", http::Status::SUCCESS_OK);
      }
    }
    catch (json::JSONParseException const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input transfer request: ", ex.what());
    }

    return BadJsonResponse(ErrorCode::PARSE_FAILURE);
  }

  /**
   * method for commencing mining the smart market
   */
  http::HTTPResponse OnMine(http::HTTPRequest const &request)
  {
    try
    {
      // parse the json request
      json::JSONDocument doc;
      doc.Parse(request.body());

      std::size_t random_seed = std::numeric_limits<std::size_t>::max();
      std::size_t run_time    = std::numeric_limits<std::size_t>::min();

      // extract all the request parameters
      if (variant::Extract(doc.root(), "random_seed", random_seed) &&
          variant::Extract(doc.root(), "run_time", run_time))
      {
        auction_.Mine(random_seed, run_time);

        return http::CreateJsonResponse(R"({"success": true})", http::Status::SUCCESS_OK);
      }
    }
    catch (json::JSONParseException const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input transfer request: ", ex.what());
    }

    return BadJsonResponse(ErrorCode::PARSE_FAILURE);
  }

  /**
   * method that executes the auction, i.e. simply prints out winning and losing bids following
   * mining
   */
  http::HTTPResponse OnExecute(http::HTTPRequest const &request)
  {
    try
    {
      // parse the json request
      json::JSONDocument doc;
      doc.Parse(request.body());

      BlockId end_block = std::numeric_limits<BlockId>::max();

      auction_.Execute();

      for (std::size_t j = 0; j < auction_.ShowBids().size(); ++j)
      {
        std::cout << "j: " << j << ", status: " << auction_.Active(j) << std::endl;
      }

      return http::CreateJsonResponse(R"({"success": true})", http::Status::SUCCESS_OK);
    }
    catch (json::JSONParseException const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input transfer request: ", ex.what());
    }

    return BadJsonResponse(ErrorCode::PARSE_FAILURE);
  }

  /**
   * method for printing error codes
   * @param error_code
   * @return
   */
  static http::HTTPResponse BadJsonResponse(ErrorCode error_code)
  {

    std::ostringstream oss;
    oss << '{' << R"("success": false,)"
        << R"("error_code": )" << static_cast<int>(error_code) << ',' << R"("message": )"
        << ToString(error_code) << '}';

    return http::CreateJsonResponse(oss.str(), http::Status::CLIENT_ERROR_BAD_REQUEST);
  }

  /**
   * helper method converting error codes to strings
   * @param error_code
   * @return
   */
  static const char *ToString(ErrorCode error_code)
  {
    char const *msg = "unknown error";

    switch (error_code)
    {
    case ErrorCode::NOT_IMPLEMENTED:
      msg = "Not implemented";
      break;
    case ErrorCode::PARSE_FAILURE:
      msg = "Parse failure";
      break;
    }

    return msg;
  }
};

}  // namespace auctions
}  // namespace fetch
