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

//#include "math/linalg/matrix.hpp"
//#include "ml/layers/layers.hpp"
//#include "ml/ops/ops.hpp"
//#include "ml/session.hpp"

//using namespace fetch::ml;
//
//using Type            = double;
//using ArrayType       = fetch::math::linalg::Matrix<Type>;
//using VariableType    = fetch::ml::Variable<ArrayType>;
//using VariablePtrType = std::shared_ptr<VariableType>;
//using LayerType       = fetch::ml::layers::Layer<ArrayType>;
//using LayerPtrType    = std::shared_ptr<LayerType>;

TEST(vickrey_auction, no_bid_auction)
{
  VickreyAuction va();

  int seller_funds = 123456;



  va.AddItem(0);

  va.execute();

  ASSERT_TRUE(va.sale_price());

}

TEST(vickrey_auction, one_bid_auction)
{
}

TEST(vickrey_auction, two_bid_auction)
{
}

TEST(vickrey_auction, many_bid_auction)
{
}
