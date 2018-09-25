//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <gmock/gmock.h>

#include "ledger/chain/wire_transaction.hpp"
#include "ledger/chain/helper_functions.hpp"
#include <memory>


namespace fetch {
namespace chain {

namespace {

  using ::testing::_;
  using namespace fetch;
  using namespace fetch::ledger;

  class WiredTransactionTest : public ::testing::Test
  {
  protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
  };

  TEST_F(WiredTransactionTest, basic)
  {
    MutableTransaction tx {RandomTransaction()};

    std::cout << "tx[before] = " << std::endl << tx << std::endl;

    //tx.set_signatures(MutableTransaction::signatures_type{}); 
    //std::cout << "tx[after] = " << tx << std::endl;
    
    auto wire_tx = ToWireTransaction(tx, true);
    std::cout << "wire tx = " << std::endl  << wire_tx << std::endl;

    auto tx_deserialised{ FromWireTransaction(wire_tx) };
    std::cout << "tx[deserialised] = " << std::endl << tx_deserialised << std::endl;
    
    //std::cout << "tx verify = " << tx.Verify() << std::endl;
  }

}  // namespace

}  // namespace ledger
}  // namespace fetch
