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
#include "ledger/chain/mutable_transaction.hpp"
#include <memory>

namespace fetch {
namespace ledger {

namespace {

using namespace fetch;
using namespace fetch::ledger;

class TxDataForSigningTest : public ::testing::Test
{

protected:
  void SetUp() override
  {}

  void TearDown() override
  {}
};

TEST_F(TxDataForSigningTest, basic_sign_verify_cycle)
{
  for (std::size_t i = 0; i < 100; ++i)
  {
    MutableTransaction tx{RandomTransaction(3, 0)};

    auto                               txdfs{TxSigningAdapterFactory(tx)};
    crypto::openssl::ECDSAPrivateKey<> key;

    tx.Sign(key.KeyAsBin(), txdfs);
    ledger::Signatory const &sig = *tx.signatures().begin();
    EXPECT_TRUE(txdfs.Verify(sig));
    EXPECT_TRUE(tx.Verify());
  }
}

TEST_F(TxDataForSigningTest, data_for_signing_are_equal_after_serialize_deserialize_cycle)
{
  for (std::size_t i = 0; i < 100; ++i)
  {
    MutableTransaction tx{RandomTransaction(3, 3)};
    tx.UpdateDigest();
    ASSERT_TRUE(tx.Verify());

    auto txdfs{TxSigningAdapterFactory(tx)};

    serializers::ByteArrayBuffer stream;
    stream << txdfs;

    MutableTransaction tx_deser;
    auto               txdfs_deser{TxSigningAdapterFactory(tx_deser)};
    stream.seek(0);
    stream >> txdfs_deser;

    tx_deser.UpdateDigest();

    EXPECT_TRUE(tx_deser.Verify());
    EXPECT_EQ(tx.digest(), tx_deser.digest());
  }
}
}  // namespace

}  // namespace ledger
}  // namespace fetch
