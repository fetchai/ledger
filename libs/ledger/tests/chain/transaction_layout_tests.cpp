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

#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_builder.hpp"
#include "ledger/chain/transaction_layout.hpp"

#include "gtest/gtest.h"

#include <memory>

using fetch::ledger::TransactionLayout;
using fetch::ledger::TransactionBuilder;
using fetch::ledger::Address;
using fetch::crypto::ECDSASigner;
using fetch::BitVector;

using SignerPtr  = std::unique_ptr<ECDSASigner>;
using AddressPtr = std::unique_ptr<Address>;

class TransactionLayoutTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    signer_  = std::make_unique<ECDSASigner>();
    address_ = std::make_unique<Address>(signer_->identity());
  }

  void TearDown() override
  {
    signer_.reset();
    address_.reset();
  }

  SignerPtr  signer_;
  AddressPtr address_;
};

TEST_F(TransactionLayoutTests, BasicTest)
{
  BitVector shard_mask{4};
  shard_mask.set(1, 1);
  shard_mask.set(2, 1);

  // build the complete transaction
  auto const tx = TransactionBuilder()
                      .From(*address_)
                      .TargetChainCode("foo.bar.baz", shard_mask)
                      .Action("action")
                      .ValidFrom(1000)
                      .ValidUntil(2000)
                      .ChargeLimit(500)
                      .Signer(signer_->identity())
                      .Seal()
                      .Sign(*signer_)
                      .Build();

  // build the transaction layout from this transaction
  TransactionLayout const layout{*tx, 2};

  EXPECT_EQ(layout.digest(), tx->digest());
  EXPECT_EQ(layout.charge(), tx->charge());
  EXPECT_EQ(layout.valid_from(), tx->valid_from());
  EXPECT_EQ(layout.valid_until(), tx->valid_until());
}