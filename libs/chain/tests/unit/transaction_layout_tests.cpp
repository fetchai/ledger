//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "chain/address.hpp"
#include "chain/transaction.hpp"
#include "chain/transaction_builder.hpp"
#include "chain/transaction_layout.hpp"
#include "core/bitvector.hpp"
#include "core/byte_array/decoders.hpp"
#include "crypto/ecdsa.hpp"

#include "gtest/gtest.h"

#include <memory>

using fetch::byte_array::FromBase64;
using fetch::chain::TransactionLayout;
using fetch::chain::TransactionBuilder;
using fetch::chain::Address;
using fetch::crypto::ECDSASigner;
using fetch::BitVector;

using SignerPtr  = std::unique_ptr<ECDSASigner>;
using AddressPtr = std::unique_ptr<Address>;

static constexpr char const *FIXED_IDENTITY = "hTgbP/9IDrscsM122fEhP5FGjqiApnkyD6LAZS2bsx4=";

class TransactionLayoutTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    signer_  = std::make_unique<ECDSASigner>();
    address_ = std::make_unique<Address>(signer_->identity());

    fixed_signer_  = std::make_unique<ECDSASigner>(FromBase64(FIXED_IDENTITY));
    fixed_address_ = std::make_unique<Address>(fixed_signer_->identity());
  }

  void TearDown() override
  {
    signer_.reset();
    address_.reset();
  }

  SignerPtr  signer_;
  AddressPtr address_;

  SignerPtr  fixed_signer_;
  AddressPtr fixed_address_;
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
  EXPECT_EQ(layout.charge_rate(), tx->charge_rate());
  EXPECT_EQ(layout.valid_from(), tx->valid_from());
  EXPECT_EQ(layout.valid_until(), tx->valid_until());
}

TEST_F(TransactionLayoutTests, FixedBasicTest)
{
  BitVector shard_mask{4};
  shard_mask.set(1, 1);
  shard_mask.set(2, 1);

  // build the complete transaction
  auto const tx = TransactionBuilder()
                      .From(*fixed_address_)
                      .TargetChainCode("foo.bar.baz", shard_mask)
                      .Action("action")
                      .ValidFrom(1000)
                      .ValidUntil(2000)
                      .ChargeLimit(500)
                      .Signer(fixed_signer_->identity())
                      .Seal()
                      .Sign(*fixed_signer_)
                      .Build();

  // build the transaction layout from this transaction
  TransactionLayout const layout{*tx, 2};

  EXPECT_EQ(layout.digest(), tx->digest());
  EXPECT_EQ(layout.charge_rate(), tx->charge_rate());
  EXPECT_EQ(layout.valid_from(), tx->valid_from());
  EXPECT_EQ(layout.valid_until(), tx->valid_until());

  EXPECT_EQ(layout.mask().bit(0), 1);
  EXPECT_EQ(layout.mask().bit(1), 1);
  EXPECT_EQ(layout.mask().bit(2), 1);
  EXPECT_EQ(layout.mask().bit(3), 0);
}
