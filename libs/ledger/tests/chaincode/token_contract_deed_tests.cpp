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
#include "ledger/chain/v2/transaction_builder.hpp"
#include "ledger/chaincode/deed.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include <gmock/gmock.h>

namespace fetch {
namespace ledger {
namespace {

using fetch::ledger::Address;
using fetch::ledger::TransactionBuilder;

using AddressArray = std::vector<Address>;
using PrivateKey   = crypto::ECDSASigner;

using byte_array::ConstByteArray;
using variant::Variant;
using Amount = uint64_t;

class TokenContractDeedTests : public ::testing::Test
{
protected:
  static AddressArray const ADDRESSES;

  static AddressArray CreateAddresses()
  {
    static constexpr std::size_t NUM_TO_GENERATE = 12;

    AddressArray addresses{};

    Address::RawAddress address{};
    for (auto &elem : address)
    {
      elem = 0;
    }

    for (std::size_t i = 0; i < NUM_TO_GENERATE; ++i)
    {
      // change the address to that it is unique
      address.back() = static_cast<uint8_t>(i);

      // add the address to the list
      addresses.emplace_back(Address{address});
    }

    return addresses;
  }

  static ConstByteArray CreateTxTransferData(ConstByteArray const &from, ConstByteArray const &to,
                                             Amount const amount)
  {
    Variant v_data{Variant::Object()};
    v_data["from"]   = from;
    v_data["to"]     = to;
    v_data["amount"] = amount;

    std::ostringstream oss;
    oss << v_data;

    return oss.str();
  }

  static TransactionBuilder::TransactionPtr CreateTransferTx(
      Address const &from, Address const &to, std::vector<PrivateKey *> const &signing_keys,
      Amount amount)
  {
    TransactionBuilder builder;
    builder.From(from);
    builder.Transfer(to, amount);

    // register all the signers
    for (auto const &signer : signing_keys)
    {
      builder.Signer(signer->identity());
    }

    // seal and sign the transaction
    auto sealed_tx = builder.Seal();

    // create all the signatures
    for (auto &signer : signing_keys)
    {
      sealed_tx.Sign(*signer);
    }

    // construct the final transaction
    return sealed_tx.Build();
  }

  static void PrintMandatoryWeights(Deed::MandatorityMatrix const &mandatory_weights)
  {
    for (auto const &op_w : mandatory_weights)
    {
      std::cout << "threshold=" << op_w.first << std::endl;
      for (auto const &w : op_w.second)
      {
        std::cout << "  w = " << w.first << " : n=" << w.second << std::endl;
      }
    }
  }
};

// static storage
AddressArray const TokenContractDeedTests::ADDRESSES{TokenContractDeedTests::CreateAddresses()};

TEST_F(TokenContractDeedTests, is_sane_basic)
{
  Deed::Signees signees;
  signees[ADDRESSES[0]] = 1;
  signees[ADDRESSES[1]] = 2;
  signees[ADDRESSES[2]] = 3;

  Deed::OperationTresholds thresholds;
  thresholds["0"] = 1;
  thresholds["1"] = 6;

  EXPECT_TRUE((Deed{signees, thresholds}.IsSane()));

  thresholds["2"] = 7;
  EXPECT_FALSE((Deed{signees, thresholds}.IsSane()));
}

TEST_F(TokenContractDeedTests, is_sane_fails_when_empty_thresholds)
{
  Deed::Signees signees;
  signees[ADDRESSES[0]] = 1;

  Deed::OperationTresholds thresholds;
  EXPECT_FALSE((Deed{signees, thresholds}.IsSane()));

  thresholds["abc"] = 1;
  EXPECT_TRUE((Deed{signees, thresholds}.IsSane()));
}

TEST_F(TokenContractDeedTests, is_sane_fails_when_empty_signees)
{
  Deed::Signees signees;

  Deed::OperationTresholds thresholds;
  thresholds["abc"] = 1;
  // Expected to **FAIL** due to empty signees
  EXPECT_FALSE((Deed{signees, thresholds}.IsSane()));

  signees[ADDRESSES[0]] = 1;
  // Proving above negative expectation by testing for the opposite:
  // Expected to **PASS**, NON-empty signees and thresholds have been provided
  EXPECT_TRUE((Deed{signees, thresholds}.IsSane()));
}

TEST_F(TokenContractDeedTests, infer_mandatory_weights)
{
  Deed::Signees signees;
  signees[ADDRESSES[0]] = 1;
  signees[ADDRESSES[1]] = 1;
  signees[ADDRESSES[2]] = 1;
  signees[ADDRESSES[3]] = 20;
  signees[ADDRESSES[4]] = 20;
  signees[ADDRESSES[5]] = 20;

  Deed::OperationTresholds thresholds;
  thresholds["a"] = 43;
  thresholds["b"] = 60;
  thresholds["c"] = 62;

  Deed deed{signees, thresholds};
  ASSERT_TRUE(deed.IsSane());

  auto const                    inferred_mandatory_weight = deed.InferMandatoryWeights();
  Deed::MandatorityMatrix const expected_mandatory_weights(
      {{43, {{20, 2}}}, {60, {{20, 3}}}, {62, {{20, 3}, {1, 2}}}});
  EXPECT_EQ(expected_mandatory_weights, inferred_mandatory_weight);
}

TEST_F(TokenContractDeedTests, infer_mandatory_weights_2)
{
  Deed::Signees signees;
  signees[ADDRESSES[0]]  = 1;
  signees[ADDRESSES[1]]  = 1;
  signees[ADDRESSES[2]]  = 1;
  signees[ADDRESSES[3]]  = 1;
  signees[ADDRESSES[4]]  = 1;
  signees[ADDRESSES[5]]  = 1;
  signees[ADDRESSES[6]]  = 2;
  signees[ADDRESSES[7]]  = 2;
  signees[ADDRESSES[8]]  = 2;
  signees[ADDRESSES[9]]  = 3;
  signees[ADDRESSES[10]] = 3;

  Deed::OperationTresholds thresholds;
  thresholds["a"] = 17;
  thresholds["b"] = 15;
  thresholds["c"] = 13;

  Deed deed{signees, thresholds};
  ASSERT_TRUE(deed.IsSane());

  auto const                    inferred_mandatory_weight = deed.InferMandatoryWeights();
  Deed::MandatorityMatrix const expected_mandatory_weights({
      {13, {{1, 1}, {2, 1}, {3, 1}}},
      {15, {{1, 3}, {2, 2}, {3, 1}}},
      {17, {{1, 5}, {2, 3}, {3, 2}}},
  });
  EXPECT_EQ(expected_mandatory_weights, inferred_mandatory_weight);
}

TEST_F(TokenContractDeedTests, is_sane_fails_when_some_thresholds_are_zero)
{
  Deed::Signees signees;
  signees[ADDRESSES[0]] = 3;

  Deed::OperationTresholds thresholds;
  thresholds["a"] = 1;
  thresholds["b"] = 0;
  thresholds["c"] = 1;
  EXPECT_FALSE((Deed{signees, thresholds}.IsSane()));

  thresholds["b"] = 1;
  EXPECT_TRUE((Deed{signees, thresholds}.IsSane()));
}

TEST_F(TokenContractDeedTests, verify_basic_scenario)
{
  std::vector<PrivateKey> keys(3);

  Address const from{keys.at(0).identity()};
  Address const to{keys.at(1).identity()};

  auto tx = CreateTransferTx(from, to, {&keys.at(0), &keys.at(2)}, 10);

  Deed::Signees signees;
  signees[Address{keys[0].identity()}] = 1;
  signees[Address{keys[1].identity()}] = 2;
  signees[Address{keys[2].identity()}] = 3;

  Deed::OperationTresholds thresholds;
  thresholds["op0"] = 1;
  thresholds["op1"] = 4;
  thresholds["op2"] = 5;

  Deed deed{signees, thresholds};
  ASSERT_TRUE(deed.IsSane());

  // This must verify SSUCCESSFULLY, since signatories 0 & 2 have accumulated
  // weight 4(=1+3) and so "op0" and "op1" thresholds (1 and 4) are in reach.
  EXPECT_TRUE(deed.Verify(*tx, "op0"));
  EXPECT_TRUE(deed.Verify(*tx, "op1"));

  // This must FAIL verification, since threshold "op2" is higher than accumulated
  // weight of signatories 0 & 2
  EXPECT_FALSE(deed.Verify(*tx, "op2"));
}

TEST_F(TokenContractDeedTests, verify_ignores_signatory_not_defined_in_deed_as_signee)
{
  std::vector<PrivateKey> keys(4);

  Address const from{keys.at(0).identity()};
  Address const to{keys.at(1).identity()};

  // Signatory 3 is NOT defined in deed as signee, and so is EXPECTED to be IGNORED.
  auto tx = CreateTransferTx(from, to, {&keys.at(0), &keys.at(3)}, 10);

  Deed::Signees signees;
  signees[Address{keys[0].identity()}] = 1;
  signees[Address{keys[1].identity()}] = 2;
  signees[Address{keys[2].identity()}] = 3;

  Deed::OperationTresholds thresholds;
  thresholds["op0"] = 1;
  thresholds["op1"] = 4;
  thresholds["op2"] = 5;

  Deed deed{signees, thresholds};
  ASSERT_TRUE(deed.IsSane());

  // This must verify SSUCCESSFULLY, since weight of signatory 0 is 1 and
  // threshold "op0" is 1.
  EXPECT_TRUE(deed.Verify(*tx, "op0"));

  // This must FAIL verification, since thresholds "op1" & "op2" are higher
  // than weight of accepted signatory 0
  EXPECT_FALSE(deed.Verify(*tx, "op1"));
  EXPECT_FALSE(deed.Verify(*tx, "op2"));
}

}  // namespace
}  // namespace ledger
}  // namespace fetch
