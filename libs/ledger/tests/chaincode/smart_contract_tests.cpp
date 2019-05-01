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
#include "crypto/sha256.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/state_adapter.hpp"

#include "contract_test.hpp"
#include "mock_storage_unit.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace {

using namespace testing;

using fetch::ledger::SmartContract;
using fetch::byte_array::ConstByteArray;
using fetch::ledger::TransactionSummary;
using fetch::storage::ResourceAddress;
using fetch::variant::Variant;
using fetch::byte_array::ToBase64;
using fetch::serializers::ByteArrayBuffer;
using ContractDigest = ConstByteArray;

template <typename T>
ConstByteArray RawBytes(T value)
{
  return ConstByteArray{reinterpret_cast<uint8_t *>(&value), sizeof(value)};
}

using SmartContractPtr   = std::unique_ptr<SmartContract>;
using MockStorageUnitPtr = std::unique_ptr<MockStorageUnit>;
using Resource           = TransactionSummary::Resource;
using Resources          = std::vector<Resource>;
using Query              = SmartContract::Query;

class SmartContractTests : public ContractTest
{
protected:
  void CreateContract(std::string const &source)
  {
    // generate the smart contract instance for this contract
    auto contract = std::make_shared<SmartContract>(source);
    contract_     = contract;
    // populate the contract name too
    contract_name_ = std::make_shared<Identifier>(ToBase64(contract->contract_digest()) + "." +
                                                  ToBase64(certificate_->identity().identifier()));

    ASSERT_TRUE(static_cast<bool>(contract_));
    ASSERT_TRUE(static_cast<bool>(contract_name_));
  }

  template <typename T>
  void VerifyQuery(ConstByteArray const &query_method_name, T const &expected_value,
                   Variant const &request = Variant::Object())
  {
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery(query_method_name, request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<T>(), expected_value);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }
};

template <typename Container, typename Key>
bool IsIn(Container const &container, Key const &key)
{
  return container.find(key) != container.end();
}

TEST_F(SmartContractTests, CheckSimpleContract)
{
  std::string const contract_source = R"(
    @action
    function increment()
      var state = State<Int32>("value", 10);
      state.set(state.get() + 1);
    endfunction

    @query
    function value() : Int32
      var state = State<Int32>("value", 10);
      return state.get();
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(1u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "increment"));

  // check the query handlers
  auto const query_handlers = contract_->query_handlers();
  ASSERT_EQ(1u, query_handlers.size());
  EXPECT_TRUE(IsIn(query_handlers, "value"));

  // define our what we expect the values to be in our storage requests
  auto const expected_key      = contract_name_->full_name() + ".state.value";
  auto const expected_resource = ResourceAddress{expected_key};
  auto const expected_value    = RawBytes<int32_t>(11);

  {
    InSequence seq;

    // from the action
    EXPECT_CALL(*storage_, Lock(expected_resource));
    EXPECT_CALL(*storage_, Get(expected_resource));
    EXPECT_CALL(*storage_, Set(expected_resource, expected_value)).Times(1);
    EXPECT_CALL(*storage_, Unlock(expected_resource));

    // from the query
    EXPECT_CALL(*storage_, Get(expected_resource));
  }

  // send the smart contract an "increment" action
  EXPECT_EQ(SmartContract::Status::OK, SendAction("increment", {"value"}));

  VerifyQuery("value", int32_t{11});
}

TEST_F(SmartContractTests, CheckQueryReturnTypes)
{
  std::string const contract_source = R"(
    @query
    function get_bool() : Bool
      return true;
    endfunction

    @query
    function get_int32() : Int32
      return 14;
    endfunction

    @query
    function get_uint32() : UInt32
      return 15u32;
    endfunction

    @query
    function get_int64() : Int64
      return 16i64;
    endfunction

    @query
    function get_uint64() : UInt64
      return 17u64;
    endfunction

    @query
    function get_float() : Float32
      return 1.0f;
    endfunction

    @query
    function get_double() : Float64
      return 2.0;
    endfunction

    @query
    function get_string() : String
      return "Why hello there";
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);
  ASSERT_TRUE(static_cast<bool>(contract_));

  VerifyQuery("get_bool", true);
  VerifyQuery("get_int32", int32_t{14});
  VerifyQuery("get_uint32", uint32_t{15});
  VerifyQuery("get_int64", int64_t{16});
  VerifyQuery("get_uint64", uint64_t{17});
  VerifyQuery("get_float", float{1.0});
  VerifyQuery("get_double", double{2.0});
  VerifyQuery("get_string", ConstByteArray{"Why hello there"});
}

TEST_F(SmartContractTests, CheckParameterizedActionAndQuery)
{
  std::string const contract_source = R"(
    @action
    function increment(increment: Int32)
      var state = State<Int32>("value", 10);
      state.set(state.get() + increment);
    endfunction

    @query
    function value() : Int32
      var state = State<Int32>("value", 10);
      return state.get();
    endfunction

    @query
    function offset(amount: Int32) : Int32
      var state = State<Int32>("value", 10);
      return state.get() + amount;
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(1u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "increment"));

  // check the query handlers
  auto const query_handlers = contract_->query_handlers();
  ASSERT_EQ(2u, query_handlers.size());
  EXPECT_TRUE(IsIn(query_handlers, "value"));
  EXPECT_TRUE(IsIn(query_handlers, "offset"));

  // define our what we expect the values to be in our storage requests
  auto const expected_key      = contract_name_->full_name() + ".state.value";
  auto const expected_resource = ResourceAddress{expected_key};
  auto const expected_value    = RawBytes<int32_t>(30);

  {
    using ::testing::_;
    InSequence seq;

    // from the action
    EXPECT_CALL(*storage_, Lock(expected_resource));
    EXPECT_CALL(*storage_, Get(expected_resource));
    EXPECT_CALL(*storage_, Set(expected_resource, expected_value)).Times(1);
    EXPECT_CALL(*storage_, Unlock(expected_resource));

    // from the query
    EXPECT_CALL(*storage_, Get(expected_resource));
    EXPECT_CALL(*storage_, Get(expected_resource));
  }

  // send the smart contract an "increment" action
  EXPECT_EQ(SmartContract::Status::OK, SendActionWithParams("increment", {"value"}, 20));

  VerifyQuery("value", int32_t{30});

  Variant request{Variant::Object()};
  request["amount"] = 100;
  VerifyQuery("offset", int32_t{130}, request);
}

TEST_F(SmartContractTests, CheckBasicTokenContract)
{
  std::string const contract_source = R"(
    @init
    function initialize(owner: Address)
        var INITIAL_SUPPLY = 100000000000u64;

        var account = State<UInt64>(owner, 0u64);
        account.set(INITIAL_SUPPLY);
    endfunction

    @action
    function transfer(from: Address, to: Address, amount: UInt64)

      // define the accounts
      var from_account = State<UInt64>(from, 0u64);
      var to_account = State<UInt64>(to, 0u64); // if new sets to 0u

      // Check if the sender has enough balance to proceed
      if (from_account.get() >= amount)
        from_account.set(from_account.get() - amount);
        to_account.set(to_account.get() + amount);
      endif

    endfunction

    @query
    function balance(address: Address) : UInt64
        var account = State<UInt64>(address, 0u64);
        return account.get();
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(1u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "transfer"));

  // check the query handlers
  auto const query_handlers = contract_->query_handlers();
  ASSERT_EQ(1u, query_handlers.size());
  EXPECT_TRUE(IsIn(query_handlers, "balance"));

  fetch::crypto::ECDSASigner target{};

  auto const owner_key =
      contract_name_->full_name() + ".state." + ToBase64(certificate_->identity().identifier());
  auto const target_key =
      contract_name_->full_name() + ".state." + ToBase64(target.identity().identifier());

  auto const owner_resource   = ResourceAddress{owner_key};
  auto const target_resource  = ResourceAddress{target_key};
  auto const initial_supply   = RawBytes<uint64_t>(100000000000ull);
  auto const transfer_amount  = RawBytes<uint64_t>(1000000000ull);
  auto const remaining_amount = RawBytes<uint64_t>(99000000000ull);

  {
    using ::testing::_;
    InSequence seq;

    // from the init
    EXPECT_CALL(*storage_, Lock(owner_resource));
    EXPECT_CALL(*storage_, Get(owner_resource));
    EXPECT_CALL(*storage_, Set(owner_resource, initial_supply));
    EXPECT_CALL(*storage_, Unlock(owner_resource));

    // from the query
    EXPECT_CALL(*storage_, Get(owner_resource));

    // from the action
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Get(_));
    EXPECT_CALL(*storage_, Get(_));
    EXPECT_CALL(*storage_, Set(_, remaining_amount));
    EXPECT_CALL(*storage_, Set(_, transfer_amount));
    EXPECT_CALL(*storage_, Unlock(_));
    EXPECT_CALL(*storage_, Unlock(_));

    // from the queries
    EXPECT_CALL(*storage_, Get(owner_resource));
    EXPECT_CALL(*storage_, Get(target_resource));
  }

  EXPECT_EQ(SmartContract::Status::OK, InvokeInit(certificate_->identity()));

  // check to see if the owners balance is present
  {
    Variant request    = Variant::Object();
    request["address"] = ToBase64(certificate_->identity().identifier());

    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("balance", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<uint64_t>(), 100000000000ull);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // send the smart contract an "increment" action
  EXPECT_EQ(SmartContract::Status::OK,
            SendActionWithParams("transfer",
                                 {ToBase64(certificate_->identity().identifier()),
                                  ToBase64(target.identity().identifier())},
                                 certificate_->identity(), target.identity(), 1000000000ull));

  Variant request{Variant::Object()};

  request["address"] = ToBase64(certificate_->identity().identifier());
  VerifyQuery("balance", 99000000000ull, request);

  request["address"] = ToBase64(target.identity().identifier());
  VerifyQuery("balance", 1000000000ull, request);
}

TEST_F(SmartContractTests, CheckPersistentMapSetAndQuery)
{
  std::string const contract_source = R"(
    @action
    function test_persistent_map()
      var state = PersistentMap<String, Int32>("value");
      state["foo"] = 20;
      state["bar"] = 30;
    endfunction

    @query
    function query_foo() : Int32
      var state = PersistentMap<String, Int32>("value");
      return state["foo"];
    endfunction

    @query
    function query_bar() : Int32
      var state = PersistentMap<String, Int32>("value");
      return state["bar"];
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(1u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "test_persistent_map"));

  // check the query handlers
  auto const query_handlers = contract_->query_handlers();
  ASSERT_EQ(2, query_handlers.size());

  // define expected values
  auto const expected_key1      = contract_name_->full_name() + ".state.value.foo";
  auto const expected_key2      = contract_name_->full_name() + ".state.value.bar";
  auto const expected_resource1 = ResourceAddress{expected_key1};
  auto const expected_resource2 = ResourceAddress{expected_key2};
  auto const expected_value1    = RawBytes<int32_t>(20);
  auto const expected_value2    = RawBytes<int32_t>(30);

  // expected calls
  EXPECT_CALL(*storage_, Lock(expected_resource1)).WillOnce(Return(true));
  EXPECT_CALL(*storage_, Lock(expected_resource2)).WillOnce(Return(true));
  EXPECT_CALL(*storage_, Set(expected_resource1, expected_value1)).WillOnce(Return());
  EXPECT_CALL(*storage_, Set(expected_resource2, expected_value2)).WillOnce(Return());
  EXPECT_CALL(*storage_, Unlock(expected_resource1)).WillOnce(Return(true));
  EXPECT_CALL(*storage_, Unlock(expected_resource2)).WillOnce(Return(true));

  // from the action & query
  EXPECT_CALL(*storage_, Get(expected_resource1))
      .WillOnce(Return(fetch::storage::Document{}))
      .WillOnce(Return(fetch::storage::Document{expected_value1}));
  EXPECT_CALL(*storage_, Get(expected_resource2))
      .WillOnce(Return(fetch::storage::Document{}))
      .WillOnce(Return(fetch::storage::Document{expected_value2}));

  // send the smart contract an "increment" action
  EXPECT_EQ(SmartContract::Status::OK,
            SendActionWithParams("test_persistent_map", {"value.bar", "value.foo"}));

  VerifyQuery("query_foo", int32_t{20});
  VerifyQuery("query_bar", int32_t{30});
}

TEST_F(SmartContractTests, CheckPersistentMapSetWithAddressAsName)
{
  std::string const contract_source = R"(
    @action
    function test_persistent_map(addr : Address)
      var state = PersistentMap<String, Int32>(addr);
      state["foo"] = 20;
    endfunction

    @query
    function query_foo(address : Address) : Int32
      var state = PersistentMap<String, Int32>(address);
      return state["foo"];
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(1u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "test_persistent_map"));

  // check the query handlers
  auto const query_handlers = contract_->query_handlers();
  ASSERT_EQ(1, query_handlers.size());

  ByteArray address_raw;
  address_raw.Resize(64);
  for (uint8_t i=0; i<address_raw.size(); ++i)
  {
    address_raw[i] = i;
  }

  Identity identity{address_raw};

  // define expected values

  auto const address_str{address_raw.ToBase64()};
  auto const expected_key1      = contract_name_->full_name() + ".state." + ToBase64(identity.identifier()) + ".foo";
  auto const expected_resource1 = ResourceAddress{expected_key1};
  auto const expected_value1    = RawBytes<int32_t>(20);

  // expected calls
  EXPECT_CALL(*storage_, Lock(expected_resource1)).WillOnce(Return(true));
  EXPECT_CALL(*storage_, Set(expected_resource1, expected_value1)).WillOnce(Return());
  EXPECT_CALL(*storage_, Unlock(expected_resource1)).WillOnce(Return(true));

  // from the action & query
  EXPECT_CALL(*storage_, Get(expected_resource1))
      .WillOnce(Return(fetch::storage::Document{}))
      .WillOnce(Return(fetch::storage::Document{expected_value1}));

  // send the smart contract an "increment" action
  EXPECT_EQ(SmartContract::Status::OK,
            SendActionWithParams("test_persistent_map", {address_str + ".foo"}, identity));

  Variant request    = Variant::Object();
  request["address"] = ToBase64(identity.identifier());
  VerifyQuery("query_foo", int32_t{20}, request);
}

}  // namespace
