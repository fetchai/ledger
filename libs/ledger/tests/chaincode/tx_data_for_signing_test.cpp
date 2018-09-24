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

#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/helper_functions.hpp"
#include <memory>


namespace fetch {
namespace chain {

namespace {

  using ::testing::_;
  using namespace fetch;
  using namespace fetch::ledger;

  class TxDataForSigningTest : public ::testing::Test
  {
  protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
  };

  TEST_F(TxDataForSigningTest, data_for_signing_are_equal_after_serialize_deserialize_cycle)
  {
    for(std::size_t i=0; i<100; ++i)
    {
      MutableTransaction tx {RandomTransaction()};
      //std::cout << "tx[before] = " << tx << std::endl;

      auto txdfs {TxDataForSigningCFactory(tx)};
    
      serializers::ByteArrayBuffer stream;
      stream << txdfs;

      MutableTransaction tx2;
      auto txdfs2 {TxDataForSigningCFactory(tx2)};
      stream.Seek(0);
      stream >> txdfs2;
      //std::cout << "tx[after] = " << tx2 << std::endl;

      EXPECT_EQ(txdfs, txdfs2);
    }
  }
}  // namespace

}  // namespace ledger
}  // namespace fetch
