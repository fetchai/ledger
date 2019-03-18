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

#include <gmock/gmock.h>

#include "ledger/chain/helper_functions.hpp"
#include "ledger/chain/wire_transaction.hpp"
#include <memory>

namespace {

using namespace fetch;
using namespace fetch::ledger;

TEST(WireTransactionTests, BasicChecks)
{
  for (std::size_t i = 0; i < 10; ++i)
  {
    MutableTransaction tx{RandomTransaction()};
    ASSERT_TRUE(tx.Verify());

    auto wire_tx = ToWireTransaction(tx, true);
    auto tx_deserialised{FromWireTransaction(wire_tx)};

    EXPECT_TRUE(tx_deserialised.Verify());

    auto const txdfs{TxSigningAdapterFactory(tx)};
    auto const txdfs_deserialised{TxSigningAdapterFactory(tx_deserialised)};
    EXPECT_EQ(txdfs, txdfs_deserialised);

    tx.UpdateDigest();
    tx_deserialised.UpdateDigest();
    EXPECT_EQ(tx.digest(), tx_deserialised.digest());
  }
}

}  // namespace
