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
#include "ledger/chain/v2/address.hpp"
#include "ledger/chain/v2/json_transaction.hpp"
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/chain/v2/transaction_builder.hpp"
#include "variant/variant.hpp"

#include "gtest/gtest.h"

#include <memory>

using fetch::crypto::ECDSASigner;
using fetch::ledger::Transaction;
using fetch::ledger::TransactionBuilder;
using fetch::ledger::Address;
using fetch::ledger::FromJsonTransaction;
using fetch::ledger::ToJsonTransaction;
using fetch::variant::Variant;

TEST(JsonTransactionTests, BasicTest)
{
  // create 2 private/public key pairs
  ECDSASigner identity1{};
  ECDSASigner identity2{};

  // create the associated addresses
  Address const address1{identity1.identity()};
  Address const address2{identity2.identity()};

  // build the single transfer transaction
  auto tx = TransactionBuilder()
                .From(address1)
                .Transfer(address2, 2000)
                .Signer(identity1.identity())
                .Seal()
                .Sign(identity1)
                .Build();

  // ensure the transaction is valid
  ASSERT_TRUE(tx->Verify());

  // build a json representation of this transaction
  Variant json{};
  ASSERT_TRUE(ToJsonTransaction(*tx, json));

  // reconstruct the transaction from the JSON
  Transaction output;
  ASSERT_TRUE(FromJsonTransaction(json, output));

  EXPECT_EQ(tx->digest(), output.digest());
  EXPECT_EQ(tx->from(), output.from());

  auto const &transfers_expected = tx->transfers();
  auto const &transfers_actual   = output.transfers();

  ASSERT_EQ(transfers_expected.size(), transfers_actual.size());
  ASSERT_FALSE(transfers_actual.empty());  // sanity

  EXPECT_EQ(transfers_expected[0].to, transfers_actual[0].to);
  EXPECT_EQ(transfers_expected[0].amount, transfers_actual[0].amount);
}