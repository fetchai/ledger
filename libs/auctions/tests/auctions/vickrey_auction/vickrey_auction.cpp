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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include <auctions/vickrey_auction.hpp>

//#include "math/linalg/matrix.hpp"
//#include "ml/layers/layers.hpp"
//#include "ml/ops/ops.hpp"
//#include "ml/session.hpp"

using namespace fetch::auctions;
//
// using Type            = double;
// using ArrayType       = fetch::math::linalg::Matrix<Type>;
// using VariableType    = fetch::ml::Variable<ArrayType>;
// using VariablePtrType = std::shared_ptr<VariableType>;
// using LayerType       = fetch::ml::layers::Layer<ArrayType>;
// using LayerPtrType    = std::shared_ptr<LayerType>;
//
// TEST(vickrey_auction, no_bid_auction)
//{
//  VickreyAuction va();
//
//  std::size_t seller_funds = 123456;
//
//  VickeryAuction::ItemIdType item = 0;
//  VickeryAuction::ValueType min_price = 10;
//
//  va.AddItem(item, min_price);
//
//
//  va.execute();
//
//  ASSERT_TRUE(va.sale_price());
//
//}
//
// TEST(vickrey_auction, one_bid_auction)
//{
//}
//
// TEST(vickrey_auction, two_bid_auction)
//{
//}

class Bidder
{
public:
  std::size_t id    = 0;
  std::size_t funds = 0;

  Bidder(std::size_t funds, std::size_t id)
    : id(id)
    , funds(funds)
  {
    ;
  }

private:
  Bidder() = default;
};

TEST(vickrey_auction, many_bid_auction)
{

  std::size_t n_bidders = 10;
  std::size_t seller_id = 999;
  std::size_t start_block = 10000;
  std::size_t end_block = start_block + 10;
  std::size_t max_items = 1;

  // set up auction
  VickreyAuction va(start_block, end_block, max_items);

  // add item to auction
  VickreyAuction::ItemIdType item      = 0;
  VickreyAuction::ValueType  min_price = 7;
  va.AddItem(item, seller_id, min_price);

  // set up bidders
  std::vector<Bidder> bidders{};
  for (std::size_t i = 0; i < n_bidders; ++i)
  {
    bidders.push_back(Bidder(i, i));
  }

  // make bids
  for (std::size_t j = 0; j < n_bidders; ++j)
  {
    va.AddSingleBid(bidders[j].funds, bidders[j].id, item);
  }

  std::size_t cur_block = start_block;
  for (std::size_t j = 0; j < 20; ++j)
  {
    va.Execute(cur_block);

    cur_block++;
  }

  ASSERT_TRUE(va.Winners()[item] == bidders[bidders.size() - 1].id);
}
