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

#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/deed.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "mock_storage_unit.hpp"

#include "contract_test.hpp"

#include <gmock/gmock.h>

#include <iostream>
#include <ledger/chain/mutable_transaction.hpp>
#include <memory>
#include <random>
#include <sstream>
#include <string>

namespace fetch {
namespace ledger {
namespace {

using ::testing::_;
using fetch::variant::Variant;
using ConstByteArray = byte_array::ConstByteArray;
using PrivateKey     = TxSigningAdapter<>::private_key_type;
using MutTx          = MutableTransaction;
using VerifTx        = VerifiedTransaction;
using PrivateKeys    = std::vector<PrivateKey>;

using SigneesPtr    = std::shared_ptr<Deed::Signees>;
using ThresholdsPtr = std::shared_ptr<Deed::OperationTresholds>;

class TokenContractTests : public ContractTest
{
protected:
  using Query              = Contract::Query;
  using TokenContractPtr   = std::unique_ptr<TokenContract>;
  using MockStorageUnitPtr = std::unique_ptr<MockStorageUnit>;
  using Address            = fetch::byte_array::ConstByteArray;

  void SetUp() override
  {
    // setup the base class
    ContractTest::SetUp();

    // create the
    contract_      = std::make_unique<TokenContract>();
    contract_name_ = std::make_shared<Identifier>(std::string{TokenContract::NAME});
  }

  static ConstByteArray CreateTxDeedData(Address const &address, SigneesPtr const signees,
                                         ThresholdsPtr const   thresholds,
                                         uint64_t const *const balance = nullptr)
  {
    Variant v_data{Variant::Object()};
    v_data["address"] = byte_array::ToBase64(address);

    if (balance)
    {
      v_data["balance"] = *balance;
    }

    if (signees)
    {
      Variant v_signees = Variant::Object();
      for (auto const &s : *signees)
      {
        v_signees[byte_array::ToBase64(s.first)] = s.second;
      }
      v_data["signees"] = v_signees;
    }

    if (thresholds)
    {
      Variant v_thresholds = Variant::Object();
      for (auto const &t : *thresholds)
      {
        v_thresholds[t.first] = t.second;
      }
      v_data["thresholds"] = v_thresholds;
    }

    std::ostringstream oss;
    oss << v_data;

    return oss.str();
  }

  static void SignTx(MutTx &tx, PrivateKeys const &keys_to_sign_tx)
  {
    auto sign_adapter{TxSigningAdapterFactory(tx)};
    for (auto const &key : keys_to_sign_tx)
    {
      tx.Sign(key, sign_adapter);
    }
  }

  bool SendDeedTx(Address const &address, PrivateKeys const &keys_to_sign_tx,
                  SigneesPtr const &signees, ThresholdsPtr const &thresholds,
                  bool const set_call_expected = true, uint64_t const *const balance = nullptr)
  {
    EXPECT_CALL(*storage_, Get(_)).Times(1);
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
    EXPECT_CALL(*storage_, Set(_, _)).Times(set_call_expected ? 1 : 0);
    EXPECT_CALL(*storage_, Lock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, Unlock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    MutTx tx;
    tx.set_contract_name("fetch.token.deed");
    tx.set_data(CreateTxDeedData(address, signees, thresholds, balance));
    tx.PushResource(address);
    SignTx(tx, keys_to_sign_tx);

    // dispatch the transaction
    auto const status = SendAction(tx);
    return (Contract::Status::OK == status);
  }

  bool CreateWealth(Address const &address, uint64_t amount)
  {
    EXPECT_CALL(*storage_, Get(_)).Times(1);
    EXPECT_CALL(*storage_, Set(_, _)).Times(1);
    EXPECT_CALL(*storage_, Lock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, Unlock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    std::ostringstream oss;
    oss << "{ "
        << R"("address": ")" << static_cast<std::string>(byte_array::ToBase64(address)) << "\", "
        << R"("amount": )" << amount << " }";

    // send the action to the contract
    auto const status = SendAction("wealth", {address}, oss.str());

    return (Contract::Status::OK == status);
  }

  bool Transfer(Address const &from, Address const &to, PrivateKeys const &keys_to_sign,
                uint64_t amount, bool const set_call_expected = true)
  {
    EXPECT_CALL(*storage_, Get(_)).Times(set_call_expected ? 2 : 1);
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
    EXPECT_CALL(*storage_, Set(_, _)).Times(set_call_expected ? 2 : 0);
    EXPECT_CALL(*storage_, Lock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, Unlock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    std::ostringstream oss;
    oss << "{ "
        << R"("from": ")" << static_cast<std::string>(byte_array::ToBase64(from)) << "\", "
        << R"("to": ")" << static_cast<std::string>(byte_array::ToBase64(to)) << "\", "
        << R"("amount": )" << amount << " }";

    // create the transaction
    MutableTransaction tx;
    tx.set_contract_name("fetch.token.transfer");
    tx.set_data(oss.str());
    tx.PushResource(from);
    tx.PushResource(to);
    SignTx(tx, keys_to_sign);

    // send the transaction to the contract
    auto const status = SendAction(tx);
    return (Contract::Status::OK == status);
  }

  bool GetBalance(Address const &address, uint64_t &balance)
  {
    EXPECT_CALL(*storage_, Get(_)).Times(1);
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
    EXPECT_CALL(*storage_, Set(_, _)).Times(0);
    EXPECT_CALL(*storage_, Lock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, Unlock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    bool success = false;

    // formulate the query
    Query query      = Variant::Object();
    query["address"] = byte_array::ToBase64(address);

    Query response;
    if (Contract::Status::OK == SendQuery("balance", query, response))
    {
      balance = response["balance"].As<uint64_t>();
      success = true;
    }

    return success;
  }
};

TEST_F(TokenContractTests, CheckWealthCreation)
{
  PrivateKey  priv_k;
  auto const &address = priv_k.publicKey().keyAsBin();

  // create wealth for this address
  EXPECT_TRUE(CreateWealth(address, 1000));

  // generate the transaction contents
  uint64_t balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, balance));
  EXPECT_EQ(balance, 1000);
}

TEST_F(TokenContractTests, CheckInitialBalance)
{
  PrivateKey  priv_k;
  auto const &address = priv_k.publicKey().keyAsBin();

  // generate the transaction contents
  uint64_t balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, balance));
  EXPECT_EQ(balance, 0);
}

TEST_F(TokenContractTests, CheckTransferWithoutPreexistingDeed)
{
  PrivateKeys keys{2};
  auto const &from = keys.at(0).publicKey().keyAsBin();
  auto const &to   = keys.at(1).publicKey().keyAsBin();

  // create wealth for the first address
  EXPECT_TRUE(CreateWealth(from, 1000));

  // transfer from wealth
  EXPECT_TRUE(Transfer(from, to, {keys.at(0)}, 400));

  uint64_t balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(from, balance));
  EXPECT_EQ(balance, 600);

  EXPECT_TRUE(GetBalance(to, balance));
  EXPECT_EQ(balance, 400);
}

TEST_F(TokenContractTests, CheckDeedCreation)
{
  PrivateKeys keys{4};
  auto const &address = keys.at(0).publicKey().keyAsBin();

  SigneesPtr signees{std::make_shared<Deed::Signees>()};
  (*signees)[keys.at(0).publicKey().keyAsBin()] = 1;
  (*signees)[keys.at(1).publicKey().keyAsBin()] = 2;
  (*signees)[keys.at(2).publicKey().keyAsBin()] = 2;

  ThresholdsPtr thresholds{std::make_shared<Deed::OperationTresholds>()};
  (*thresholds)["transfer"] = 3;
  (*thresholds)["amend"]    = 5;

  // EXPECTED to **FAIL**, because of wrong signatory provided (3 instead of 0)
  EXPECT_FALSE(SendDeedTx(address, {keys.at(3)}, signees, thresholds, false));

  // EXPECTED to **PASS**, neccesary&sufficient signatory 0 provided (corresponds to `address`)
  EXPECT_TRUE(SendDeedTx(address, {keys.at(0)}, signees, thresholds));

  uint64_t balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, balance));
  EXPECT_EQ(balance, 0);
}

TEST_F(TokenContractTests, CheckDeedAmend)
{
  PrivateKeys keys{4};
  auto const &address = keys.at(0).publicKey().keyAsBin();

  // PRE-CONDITION: Create DEED
  SigneesPtr signees{std::make_shared<Deed::Signees>()};
  (*signees)[keys.at(0).publicKey().keyAsBin()] = 2;
  (*signees)[keys.at(1).publicKey().keyAsBin()] = 5;
  (*signees)[keys.at(2).publicKey().keyAsBin()] = 5;

  ThresholdsPtr thresholds{std::make_shared<Deed::OperationTresholds>()};
  (*thresholds)["transfer"] = 7;
  (*thresholds)["amend"]    = 12;

  ASSERT_TRUE(SendDeedTx(address, {keys.at(0)}, signees, thresholds));

  // TEST OBJECTIVE: Modify deed
  SigneesPtr signees_modif{std::make_shared<Deed::Signees>()};
  (*signees_modif)[keys.at(0).publicKey().keyAsBin()] = 1;
  (*signees_modif)[keys.at(1).publicKey().keyAsBin()] = 1;
  (*signees_modif)[keys.at(2).publicKey().keyAsBin()] = 2;
  (*signees_modif)[keys.at(3).publicKey().keyAsBin()] = 2;

  ThresholdsPtr thresholds_modif{std::make_shared<Deed::OperationTresholds>()};
  (*thresholds_modif)["transfer"] = 5;
  (*thresholds_modif)["amend"]    = 6;

  // EXPECTED to **FAIL** due to insufficient voting power (=> deed has **NOT** been modified)
  EXPECT_FALSE(
      SendDeedTx(address, {keys.at(1), keys.at(2)}, signees_modif, thresholds_modif, false));

  // EXPECTED TO **PASS** (sufficient amount of signatories provided => deed will be modified)
  EXPECT_TRUE(
      SendDeedTx(address, {keys.at(0), keys.at(1), keys.at(2)}, signees_modif, thresholds_modif));
}

TEST_F(TokenContractTests, CheckDeedDeletion)
{
  uint64_t const origina_wealth  = 1000;
  uint64_t const transfer_amount = 400;
  PrivateKeys    keys{4};
  auto const &   address    = keys.at(0).publicKey().keyAsBin();
  auto const &   to_address = keys.at(1).publicKey().keyAsBin();

  // 1st PRE-CONDITION: Create WEALTH
  ASSERT_TRUE(CreateWealth(address, origina_wealth));

  // 2nd PRE-CONDITION: Create DEED
  SigneesPtr signees{std::make_shared<Deed::Signees>()};
  (*signees)[keys.at(0).publicKey().keyAsBin()] = 2;
  (*signees)[keys.at(1).publicKey().keyAsBin()] = 5;
  (*signees)[keys.at(2).publicKey().keyAsBin()] = 5;

  ThresholdsPtr thresholds{std::make_shared<Deed::OperationTresholds>()};
  (*thresholds)["transfer"] = 7;
  (*thresholds)["amend"]    = 12;

  ASSERT_TRUE(SendDeedTx(address, {keys.at(0)}, signees, thresholds));

  // PROVING that DEED is in EFFECT by executing 2 TRANSFERS - first transfer
  // shall fail nad 2nd transfer shall pass:
  // EXPECTED to **FAIL** - transfer is intentionally configured as deed would
  // NOT be in effect (= providing only single signature for FROM address what
  // would be sufficient **IF** deed would NOT be in effect):
  ASSERT_FALSE(Transfer(address, to_address, {keys.at(0)}, transfer_amount, false));
  uint64_t balance = std::numeric_limits<uint64_t>::max();
  ASSERT_TRUE(GetBalance(address, balance));
  ASSERT_EQ(origina_wealth, balance);
  // EXPECTED to **PASS**: 2nd transfer cofigured to conform with deed and so it
  // shall pass:
  ASSERT_TRUE(Transfer(address, to_address, {keys.at(0), keys.at(1)}, 400));
  balance = std::numeric_limits<uint64_t>::max();
  ASSERT_TRUE(GetBalance(address, balance));
  ASSERT_EQ(origina_wealth - transfer_amount, balance);

  // TESTS OBJECTIVE: Deletion of the DEED
  // EXPECTED TO **PASS**
  EXPECT_TRUE(
      SendDeedTx(address, {keys.at(0), keys.at(1), keys.at(2)}, SigneesPtr{}, ThresholdsPtr{}));

  // PROVING THAT DEED HAS BEEN DELETED:
  // EXPECTED to **FAIL** - Proving that transfer int not possible to perform
  // without at least one signature, e.g. if we would have for some reason the
  // "empty" deed in effect (= deed would be on record but would contain empty
  // container of signees):
  EXPECT_FALSE(Transfer(address, to_address, {}, transfer_amount, false));
  // EXPECTED to **PASS** - Transfer is intentionally configured as deed would
  // NOT be in effect (= providing only single signature for FROM address what
  // shall be sufficient to modify the balance if deed has been deleted):
  EXPECT_TRUE(Transfer(address, to_address, {keys.at(0)}, transfer_amount));
  balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, balance));
  EXPECT_EQ(origina_wealth - transfer_amount - transfer_amount, balance);
}

TEST_F(TokenContractTests, CheckDeedAmendDoesNotAffectBallance)
{
  PrivateKeys keys{4};
  auto const &address = keys.at(0).publicKey().keyAsBin();

  // PRE-CONDITION: Create DEED
  SigneesPtr signees{std::make_shared<Deed::Signees>()};
  (*signees)[keys.at(0).publicKey().keyAsBin()] = 2;
  (*signees)[keys.at(1).publicKey().keyAsBin()] = 5;
  (*signees)[keys.at(2).publicKey().keyAsBin()] = 5;

  ThresholdsPtr thresholds{std::make_shared<Deed::OperationTresholds>()};
  (*thresholds)["transfer"] = 7;
  (*thresholds)["amend"]    = 12;

  ASSERT_TRUE(SendDeedTx(address, {keys.at(0)}, signees, thresholds));
  uint64_t orig_balance = std::numeric_limits<uint64_t>::max();
  ASSERT_TRUE(GetBalance(address, orig_balance));
  ASSERT_EQ(orig_balance, 0);

  // TEST OBJECTIVE: Modify deed
  SigneesPtr signees_modif{std::make_shared<Deed::Signees>()};
  (*signees_modif)[keys.at(0).publicKey().keyAsBin()] = 1;
  (*signees_modif)[keys.at(1).publicKey().keyAsBin()] = 1;
  (*signees_modif)[keys.at(2).publicKey().keyAsBin()] = 2;
  (*signees_modif)[keys.at(3).publicKey().keyAsBin()] = 2;

  ThresholdsPtr thresholds_modif{std::make_shared<Deed::OperationTresholds>()};
  (*thresholds_modif)["transfer"] = 5;
  (*thresholds_modif)["amend"]    = 6;

  uint64_t const new_balance{12345};
  // EXPECTED to **FAIL** since Tx deed json carries unexpected element(s) (the `balance`)
  EXPECT_FALSE(SendDeedTx(address, keys, signees_modif, thresholds_modif, false, &new_balance));

  // Balance MUST remain UNCHANGED
  uint64_t current_balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, current_balance));
  EXPECT_EQ(orig_balance, current_balance);
}

TEST_F(TokenContractTests, CheckTransferIsAuthorisedByPreexistingDeed)
{
  PrivateKeys    keys{3};
  auto const &   address    = keys.at(0).publicKey().keyAsBin();
  auto const &   to_address = keys.at(1).publicKey().keyAsBin();
  uint64_t const starting_balance{1000};

  // 1st PRE-CONDITION: Create wealth
  ASSERT_TRUE(CreateWealth(address, starting_balance));
  uint64_t balance = std::numeric_limits<uint64_t>::max();
  ASSERT_TRUE(GetBalance(address, balance));
  ASSERT_EQ(starting_balance, balance);

  // 2nd PRE-CONDITION: Create DEED
  SigneesPtr signees{std::make_shared<Deed::Signees>()};
  (*signees)[keys.at(0).publicKey().keyAsBin()] = 2;
  (*signees)[keys.at(1).publicKey().keyAsBin()] = 5;
  (*signees)[keys.at(2).publicKey().keyAsBin()] = 5;

  ThresholdsPtr thresholds{std::make_shared<Deed::OperationTresholds>()};
  (*thresholds)["transfer"] = 7;
  (*thresholds)["amend"]    = 12;

  ASSERT_TRUE(SendDeedTx(address, {keys.at(0)}, signees, thresholds));
  uint64_t curr_balance = std::numeric_limits<uint64_t>::max();
  ASSERT_TRUE(GetBalance(address, curr_balance));
  ASSERT_EQ(starting_balance, curr_balance);

  // TEST OBJECTIVE: Transfer is controlled by pre-existing deed

  uint64_t const transferred_amount{400};
  // EXPECTED TO **FAIL** due to insufficient voting power
  EXPECT_FALSE(Transfer(address, to_address, {keys.at(2)}, transferred_amount, false));
  // EXPECTED TO **PASS** : sufficient voting power
  EXPECT_TRUE(Transfer(address, to_address, {keys.at(1), keys.at(2)}, transferred_amount));

  balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, balance));
  EXPECT_EQ(starting_balance - transferred_amount, balance);

  EXPECT_TRUE(GetBalance(to_address, balance));
  EXPECT_EQ(transferred_amount, balance);
}

}  // namespace
}  // namespace ledger
}  // namespace fetch
