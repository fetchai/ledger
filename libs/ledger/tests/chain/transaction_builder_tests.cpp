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

#include "crypto/identity.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/v2/address.hpp"
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/chain/v2/transaction_builder.hpp"

#include <gtest/gtest.h>

#include <memory>

using fetch::ledger::v2::TransactionBuilder;
using fetch::ledger::v2::Address;
using fetch::crypto::ECDSASigner;

using BuilderPtr = std::unique_ptr<TransactionBuilder>;

class TransactionBuilderTests : public ::testing::Test
{
protected:

};

TEST_F(TransactionBuilderTests, BasicTest)
{
//  ECDSASigner signer;
//  Address signer_address{signer.identity()};
//
//  // build the transaction
//  auto tx = TransactionBuilder()
//    .From(signer_address)
//    .ValidFrom(10)
//    .ValidUntil(20)
//    .ChargeRate(1)
//    .ChargeLimit(200)
//    .Signer(signer.identity())
//    .Seal()
//    .Sign(signer)
//    .Build();
//
//  ASSERT_TRUE(static_cast<bool>(tx));
//  EXPECT_EQ(signer_address, tx->from());
//  EXPECT_EQ(10, tx->valid_from());
//  EXPECT_EQ(20, tx->valid_until());
//  EXPECT_EQ(1, tx->charge());
//  EXPECT_EQ(200, tx->charge_limit());
}
