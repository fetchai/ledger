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

#include <cstddef>
#include <numeric>
#include <vector>

#include "transaction.hpp"

namespace fetch {
namespace auctions {
namespace example {

class MockLedger
{
public:
  MockLedger()
    : cur_block_id_(0){};

  void AddNewBlock()
  {
    //    blocks_.push_back(txs);
    ++cur_block_id_;
  }

  size_t CurBlockNum()
  {
    return cur_block_id_;
  }

private:
  std::vector<std::vector<Transaction>> blocks_{};
  size_t                                cur_block_id_ = std::numeric_limits<size_t>::max();
};

}  // namespace example
}  // namespace auctions
}  // namespace fetch