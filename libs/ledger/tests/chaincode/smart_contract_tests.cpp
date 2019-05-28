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
#include "ledger/chain/transaction_builder.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/state_adapter.hpp"

#include "contract_test.hpp"
#include "mock_storage_unit.hpp"

#include <gmock/gmock.h>

#include <memory>

namespace {

using ::testing::_;
using ::testing::InSequence;
using fetch::crypto::SHA256;
using fetch::ledger::SmartContract;
using fetch::byte_array::ConstByteArray;
using fetch::storage::ResourceAddress;
using fetch::variant::Variant;
using fetch::serializers::ByteArrayBuffer;
using fetch::ledger::Address;

using ContractDigest = ConstByteArray;

ConstByteArray DigestOf(ConstByteArray const &value)
{
  class SHA256 sha;
  sha.Update(value);
  return sha.Final();
}

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
    // calculate the digest of the contract
    auto const contract_digest = DigestOf(source);

    // generate the smart contract instance for this contract
    contract_         = std::make_unique<SmartContract>(source);
    contract_address_ = std::make_unique<Address>(contract_digest);

    // populate the contract name too
    contract_name_ = std::make_shared<Identifier>(contract_address_->address().ToHex() + "." +
                                                  owner_address_->display());

    ASSERT_TRUE(static_cast<bool>(contract_));
    ASSERT_TRUE(static_cast<bool>(contract_name_));
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
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Get(expected_resource));
    EXPECT_CALL(*storage_, Set(expected_resource, expected_value)).Times(1);
    EXPECT_CALL(*storage_, Unlock(_));

    // from the query
    EXPECT_CALL(*storage_, Get(expected_resource));
  }

  // send the smart contract an "increment" action
  EXPECT_EQ(SmartContract::Status::OK, SendSmartAction("increment"));

  // make the query
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("value", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<int32_t>(), 11);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }
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
      return 1.0;
    endfunction

    @query
    function get_string() : String
      return "Why hello there";
    endfunction
  )";

  // create the contract
  CreateContract(contract_source);
  ASSERT_TRUE(static_cast<bool>(contract_));

  // check bool
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_bool", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<bool>(), true);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // check int32
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_int32", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<int32_t>(), 14);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // check uint32
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_uint32", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<uint32_t>(), 15);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // check int64
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_int64", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<int64_t>(), 16);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // check uint64
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_uint64", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<uint64_t>(), 17);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // check float
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_float", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_FLOAT_EQ(response["result"].As<float>(), 1.0f);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // check double
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_double", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_FLOAT_EQ(response["result"].As<float>(), 1.0);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // check string
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("get_string", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<ConstByteArray>(), "Why hello there");

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }
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
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Get(expected_resource));
    EXPECT_CALL(*storage_, Set(expected_resource, expected_value)).Times(1);
    EXPECT_CALL(*storage_, Unlock(_));

    // from the query
    EXPECT_CALL(*storage_, Get(expected_resource));
    EXPECT_CALL(*storage_, Get(expected_resource));
  }

  // send the smart contract an "increment" action
  EXPECT_EQ(SmartContract::Status::OK, SendSmartActionWithParams("increment", 20));

  // make the query
  {
    Variant request = Variant::Object();
    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("value", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<int32_t>(), 30);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // make the query
  {
    Variant request   = Variant::Object();
    request["amount"] = 100;

    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("offset", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<int32_t>(), 130);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }
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
  fetch::ledger::Address     target_address{target.identity()};

  auto const owner_key  = contract_name_->full_name() + ".state." + owner_address_->display();
  auto const target_key = contract_name_->full_name() + ".state." + target_address.display();

  auto const owner_resource   = ResourceAddress{owner_key};
  auto const target_resource  = ResourceAddress{target_key};
  auto const initial_supply   = RawBytes<uint64_t>(100000000000ull);
  auto const transfer_amount  = RawBytes<uint64_t>(1000000000ull);
  auto const remaining_amount = RawBytes<uint64_t>(99000000000ull);

  {
    using ::testing::_;
    InSequence seq;

    // from the init
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Get(owner_resource));
    EXPECT_CALL(*storage_, Set(owner_resource, initial_supply));
    EXPECT_CALL(*storage_, Unlock(_));

    // from the query
    EXPECT_CALL(*storage_, Get(owner_resource));

    // from the action
    EXPECT_CALL(*storage_, Lock(_));
    EXPECT_CALL(*storage_, Get(_));
    EXPECT_CALL(*storage_, Get(_));
    EXPECT_CALL(*storage_, Set(_, remaining_amount));
    EXPECT_CALL(*storage_, Set(_, transfer_amount));
    EXPECT_CALL(*storage_, Unlock(_));

    // from the queries
    EXPECT_CALL(*storage_, Get(owner_resource));
    EXPECT_CALL(*storage_, Get(target_resource));
  }

  EXPECT_EQ(SmartContract::Status::OK, InvokeInit(certificate_->identity()));

  // check to see if the owners balance is present
  {
    Variant request    = Variant::Object();
    request["address"] = owner_address_->display();

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
            SendSmartActionWithParams("transfer", *owner_address_, target_address, 1000000000ull));

  // make the query
  {
    Variant request    = Variant::Object();
    request["address"] = owner_address_->display();

    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("balance", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<uint64_t>(), 99000000000ull);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }

  // make the query
  {
    Variant request    = Variant::Object();
    request["address"] = target_address.display();

    Variant response;
    EXPECT_EQ(SmartContract::Status::OK, SendQuery("balance", request, response));

    // check the response is as we expect
    ASSERT_TRUE(response.Has("result"));
    EXPECT_EQ(response["result"].As<uint64_t>(), 1000000000ull);

    ASSERT_TRUE(response.Has("status"));
    EXPECT_EQ(response["status"].As<ConstByteArray>(), "success");
  }
}

}  // namespace
