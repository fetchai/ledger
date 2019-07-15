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

#include "contract_test.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/transaction_builder.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/state_adapter.hpp"
#include "mock_storage_unit.hpp"

#include "gmock/gmock.h"

#include <memory>

namespace {

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

using fetch::ledger::SmartContract;
using fetch::byte_array::ConstByteArray;
using fetch::storage::ResourceAddress;
using fetch::variant::Variant;
using fetch::serializers::ByteArrayBuffer;
using fetch::ledger::Address;
using ContractDigest = ConstByteArray;

template <typename T>
ConstByteArray RawBytes(T value)
{
  return ConstByteArray{reinterpret_cast<uint8_t *>(&value), sizeof(value)};
}

using SmartContractPtr   = std::unique_ptr<SmartContract>;
using MockStorageUnitPtr = std::unique_ptr<MockStorageUnit>;
using Query              = SmartContract::Query;

class SmartContractTests : public ContractTest
{
protected:
  void CreateContract(std::string const &source)
  {
    // generate the smart contract instance for this contract
    auto contract     = std::make_shared<SmartContract>(source);
    contract_         = contract;
    contract_address_ = std::make_unique<Address>(contract->contract_digest());
    // populate the contract name too
    contract_name_ = std::make_shared<Identifier>(contract_address_->address().ToHex() + "." +
                                                  owner_address_->display());

    ASSERT_TRUE(static_cast<bool>(contract_));
    ASSERT_TRUE(static_cast<bool>(contract_name_));
  }

  template <typename T>
  void VerifyQuery(ConstByteArray const &query_method_name, T const &expected_value,
                   Variant const &request = Variant::Object())
  {
    Variant response;
    EXPECT_EQ(SmartContract::eStatus::OK, SendQuery(query_method_name, request, response));

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
      var state = State<Int32>("value");
      state.set(11);
    endfunction

    @query
    function value() : Int32
      var state = State<Int32>("value");
      return state.get(0i32);
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
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Set(expected_resource, expected_value));
    EXPECT_CALL(*storage_, Unlock(_));

    // from the query
    EXPECT_CALL(*storage_, Get(expected_resource)).Times(2);
  }

  // send the smart contract an "increment" action
  auto const status{SendSmartAction("increment")};
  EXPECT_EQ(SmartContract::eStatus::OK, status.status);

  VerifyQuery("value", int32_t{11});
}

TEST_F(SmartContractTests, CheckActionResult)
{
  std::string const contract_source = R"(
    @action
    function does_not_return()
    endfunction

    @action
    function returns_Int64() : Int64
      return 123i64;
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(2u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "does_not_return"));
  EXPECT_TRUE(IsIn(transaction_handlers, "returns_Int64"));

  {
    InSequence seq;

    // from the action
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Unlock(_));
  }

  // send the smart contract an "increment" action
  auto const status_0{SendSmartAction("does_not_return")};
  EXPECT_EQ(SmartContract::eStatus::OK, status_0.status);
  EXPECT_EQ(0ll, status_0.return_value);

  {
    InSequence seq;

    // from the action
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Unlock(_));
  }

  // send the smart contract an "increment" action
  auto const status_1{SendSmartAction("returns_Int64")};
  EXPECT_EQ(SmartContract::eStatus::OK, status_1.status);
  EXPECT_EQ(123ll, status_1.return_value);
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
      var state = State<Int32>("value");
      state.set(10 + increment);
    endfunction

    @query
    function value() : Int32
      var state = State<Int32>("value");
      return state.get(0);
    endfunction

    @query
    function offset(amount: Int32) : Int32
      var state = State<Int32>("value");
      return state.get(0) + amount;
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
    InSequence seq;

    // from the action
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Set(expected_resource, expected_value));
    EXPECT_CALL(*storage_, Unlock(_));

    // from the `value` query
    EXPECT_CALL(*storage_, Get(expected_resource)).Times(2);

    // from the `offset` query
    EXPECT_CALL(*storage_, Get(expected_resource)).Times(2);
  }

  // send the smart contract an "increment" action
  auto const status{SendSmartActionWithParams("increment", 20)};
  EXPECT_EQ(SmartContract::eStatus::OK, status.status);

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
        State<UInt64>(owner).set(INITIAL_SUPPLY);
    endfunction

    @action
    function transfer(from: Address, to: Address, amount: UInt64)

      // define the accounts
      var from_account = State<UInt64>(from);
      var to_account = State<UInt64>(to); // if new sets to 0u

      // Check if the sender has enough balance to proceed
      if (from_account.get(0u64) >= amount)
        from_account.set(from_account.get(0u64) - amount);
        to_account.set(to_account.get(0u64) + amount);
      endif

    endfunction

    @query
    function balance(address: Address) : UInt64
        return State<UInt64>(address).get(0u64);
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
  fetch::ledger::Address     target_address{target.identity()};

  auto const owner_key  = contract_name_->full_name() + ".state." + owner_address_->display();
  auto const target_key = contract_name_->full_name() + ".state." + target_address.display();

  auto const owner_resource   = ResourceAddress{owner_key};
  auto const target_resource  = ResourceAddress{target_key};
  auto const default_val      = RawBytes<uint64_t>(0ull);
  auto const initial_supply   = RawBytes<uint64_t>(100000000000ull);
  auto const transfer_amount  = RawBytes<uint64_t>(1000000000ull);
  auto const remaining_amount = RawBytes<uint64_t>(99000000000ull);

  {
    InSequence seq;

    // from the init
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Set(owner_resource, initial_supply));
    EXPECT_CALL(*storage_, Unlock(_));

    // from query
    EXPECT_CALL(*storage_, Get(owner_resource)).Times(2);  // from io.Exists() & io.Read()

    // from the action
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Get(owner_resource));                    // from io.Exists()
    EXPECT_CALL(*storage_, Get(owner_resource));                    // from io.Read()
    EXPECT_CALL(*storage_, Get(target_resource));                   // from io.Exists()
    EXPECT_CALL(*storage_, Set(target_resource, transfer_amount));  // from io.Write()
    EXPECT_CALL(*storage_, Set(owner_resource, remaining_amount));  // from io.Write()
    EXPECT_CALL(*storage_, Unlock(_));

    // from query
    EXPECT_CALL(*storage_, Get(owner_resource)).Times(2);

    // from query
    EXPECT_CALL(*storage_, Get(target_resource)).Times(2);
  }

  auto const status_1{InvokeInit(certificate_->identity())};
  EXPECT_EQ(SmartContract::eStatus::OK, status_1.status);

  // make the query
  {
    auto request{Variant::Object()};
    request["address"] = owner_address_->display();
    VerifyQuery("balance", int64_t{100000000000ull}, request);
  }

  // send the smart contract an "increment" action
  auto const status_2{
      SendSmartActionWithParams("transfer", *owner_address_, target_address, 1000000000ull)};
  EXPECT_EQ(SmartContract::eStatus::OK, status_2.status);

  // make the query
  {
    auto request{Variant::Object()};
    request["address"] = owner_address_->display();
    VerifyQuery("balance", int64_t{99000000000ull}, request);
  }

  // make the query
  {
    auto request{Variant::Object()};
    request["address"] = target_address.display();
    VerifyQuery("balance", int64_t{1000000000ull}, request);
  }
}

TEST_F(SmartContractTests, CheckShardedStateSetAndQuery)
{
  std::string const contract_source = R"(
    @action
    function test_sharded_state()
      var state = ShardedState<Int32>("value");
      state.set("foo", 20);
      state.set("bar", 30);
    endfunction

    @query
    function query_foo() : Int32
      var state = ShardedState<Int32>("value");
      return state.get("foo", 0i32);
    endfunction

    @query
    function query_bar() : Int32
      var state = ShardedState<Int32>("value");
      return state.get("bar", 0i32);
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(1u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "test_sharded_state"));

  // check the query handlers
  auto const query_handlers = contract_->query_handlers();
  ASSERT_EQ(2, query_handlers.size());

  // define expected values
  auto const       expected_key1      = contract_name_->full_name() + ".state.value.foo";
  auto const       expected_key2      = contract_name_->full_name() + ".state.value.bar";
  auto const       expected_resource1 = ResourceAddress{expected_key1};
  auto const       expected_resource2 = ResourceAddress{expected_key2};
  auto const       expected_value1    = RawBytes<int32_t>(20);
  auto const       expected_value2    = RawBytes<int32_t>(30);
  fetch::BitVector mask{1ull << 4};
  auto const       lane1 = expected_resource1.lane(mask.log2_size());
  auto const       lane2 = expected_resource2.lane(mask.log2_size());
  mask.set(lane1, 1);
  mask.set(lane2, 1);
  shards(mask);

  // expected calls
  EXPECT_CALL(*storage_, Lock(lane1)).WillOnce(Return(true));
  EXPECT_CALL(*storage_, Unlock(lane1)).WillOnce(Return(true));
  if (lane1 != lane2)
  {
    EXPECT_CALL(*storage_, Lock(lane2)).WillOnce(Return(true));
    EXPECT_CALL(*storage_, Unlock(lane2)).WillOnce(Return(true));
  }

  EXPECT_CALL(*storage_, Set(expected_resource1, expected_value1)).WillOnce(Return());
  EXPECT_CALL(*storage_, Set(expected_resource2, expected_value2)).WillOnce(Return());

  // from the action & query
  EXPECT_CALL(*storage_, Get(expected_resource1))
      .WillOnce(Return(fetch::storage::Document{}))
      .WillOnce(Return(fetch::storage::Document{expected_value1}));
  EXPECT_CALL(*storage_, Get(expected_resource2))
      .WillOnce(Return(fetch::storage::Document{}))
      .WillOnce(Return(fetch::storage::Document{expected_value2}));

  // send the smart contract an "increment" action
  auto const status{SendSmartAction("test_sharded_state")};
  EXPECT_EQ(SmartContract::eStatus::OK, status.status);

  VerifyQuery("query_foo", int32_t{20});
  VerifyQuery("query_bar", int32_t{30});
}

TEST_F(SmartContractTests, CheckShardedStateSetWithAddressAsName)
{
  std::string const contract_source = R"(
    @action
    function test_sharded_state(address : Address)
      var state = ShardedState<Int32>(address);
      state.set("foo", 20);
    endfunction

    @query
    function query_foo(address : Address) : Int32
      var state = ShardedState<Int32>(address);
      return state.get("foo", 0i32);
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);

  // check the registered handlers
  auto const transaction_handlers = contract_->transaction_handlers();
  ASSERT_EQ(1u, transaction_handlers.size());
  EXPECT_TRUE(IsIn(transaction_handlers, "test_sharded_state"));

  // check the query handlers
  auto const query_handlers = contract_->query_handlers();
  ASSERT_EQ(1, query_handlers.size());

  ByteArray address_raw;
  address_raw.Resize(64);
  for (uint8_t i = 0; i < address_raw.size(); ++i)
  {
    address_raw[i] = i;
  }

  Address address_as_name{Identity{address_raw}};

  // define expected values
  auto const expected_key1 =
      contract_name_->full_name() + ".state." + address_as_name.display() + ".foo";
  auto const       expected_resource1 = ResourceAddress{expected_key1};
  auto const       expected_value1    = RawBytes<int32_t>(20);
  fetch::BitVector mask{1ull << 4};
  auto const       lane1 = expected_resource1.lane(mask.log2_size());
  mask.set(lane1, 1);
  shards(mask);

  // expected calls
  EXPECT_CALL(*storage_, Lock(lane1)).WillOnce(Return(true));
  EXPECT_CALL(*storage_, Set(expected_resource1, expected_value1)).WillOnce(Return());
  EXPECT_CALL(*storage_, Unlock(lane1)).WillOnce(Return(true));

  // from the action & query
  EXPECT_CALL(*storage_, Get(expected_resource1))
      .WillOnce(Return(fetch::storage::Document{}))
      .WillOnce(Return(fetch::storage::Document{expected_value1}));

  // send the smart contract an "increment" action
  auto const status{SendSmartActionWithParams("test_sharded_state", address_as_name)};
  EXPECT_EQ(SmartContract::eStatus::OK, status.status);

  auto request{Variant::Object()};
  request["address"] = address_as_name.display();
  VerifyQuery("query_foo", int32_t{20}, request);
}

}  // namespace
