#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/make_unique.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "mock_storage_unit.hpp"

#include <gmock/gmock.h>

#include <iostream>
#include <ledger/chain/mutable_transaction.hpp>
#include <memory>
#include <random>
#include <sstream>
#include <string>

using ::testing::_;
using namespace fetch;
using namespace fetch::ledger;

class TokenContractTests : public ::testing::Test
{
protected:
  using query_type    = Contract::query_type;
  using contract_type = std::unique_ptr<TokenContract>;
  using storage_type  = std::unique_ptr<MockStorageUnit>;
  using address_type  = fetch::byte_array::ConstByteArray;

  enum
  {
    IDENTITY_SIZE = 64
  };

  void SetUp() override
  {
    contract_ = fetch::make_unique<TokenContract>();
    storage_  = fetch::make_unique<MockStorageUnit>();

    contract_->Attach(*storage_);
  }

  bool CreateWealth(address_type const &address, uint64_t amount)
  {

    EXPECT_CALL(*storage_, Get(_)).Times(0);
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(1);
    EXPECT_CALL(*storage_, Set(_, _)).Times(1);
    EXPECT_CALL(*storage_, Lock(_)).Times(1);
    EXPECT_CALL(*storage_, Unlock(_)).Times(1);
    EXPECT_CALL(*storage_, Hash()).Times(0);
    EXPECT_CALL(*storage_, Commit(_)).Times(0);
    EXPECT_CALL(*storage_, Revert(_)).Times(0);
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    std::ostringstream oss;
    oss << "{ "
        << R"("address": ")"
        << static_cast<std::string>(byte_array::ToBase64(address)) << "\", "
        << R"("amount": )" << amount << " }";

    // create the transaction
    chain::MutableTransaction tx;
    tx.set_contract_name("fetch.token.wealth");
    tx.set_data(oss.str());
    tx.PushResource(address);

    Identifier identifier;
    identifier.Parse(static_cast<std::string>(tx.contract_name()));

    // dispatch the transaction
    auto status = contract_->DispatchTransaction(
        identifier.name(), chain::VerifiedTransaction::Create(tx));

    return (Contract::Status::OK == status);
  }

  bool Transfer(address_type const &from, address_type const &to,
                uint64_t amount)
  {

    EXPECT_CALL(*storage_, Get(_)).Times(1);
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(1);
    EXPECT_CALL(*storage_, Set(_, _)).Times(2);
    EXPECT_CALL(*storage_, Lock(_)).Times(2);
    EXPECT_CALL(*storage_, Unlock(_)).Times(2);
    EXPECT_CALL(*storage_, Hash()).Times(0);
    EXPECT_CALL(*storage_, Commit(_)).Times(0);
    EXPECT_CALL(*storage_, Revert(_)).Times(0);
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    std::ostringstream oss;
    oss << "{ "
        << R"("from": ")"
        << static_cast<std::string>(byte_array::ToBase64(from)) << "\", "
        << R"("to": ")" << static_cast<std::string>(byte_array::ToBase64(to))
        << "\", "
        << R"("amount": )" << amount << " }";

    // create the transaction
    chain::MutableTransaction tx;
    tx.set_contract_name("fetch.token.transfer");
    tx.set_data(oss.str());
    tx.PushResource(from);
    tx.PushResource(to);

    // dispatch the transaction
    Identifier identifier;
    identifier.Parse(static_cast<std::string>(tx.contract_name()));

    auto status = contract_->DispatchTransaction(
        identifier.name(), chain::VerifiedTransaction::Create(tx));
    return (Contract::Status::OK == status);
  }

  bool GetBalance(address_type const &address, uint64_t &balance)
  {

    EXPECT_CALL(*storage_, Get(_)).Times(1);
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
    EXPECT_CALL(*storage_, Set(_, _)).Times(0);
    EXPECT_CALL(*storage_, Lock(_)).Times(0);
    EXPECT_CALL(*storage_, Unlock(_)).Times(0);
    EXPECT_CALL(*storage_, Hash()).Times(0);
    EXPECT_CALL(*storage_, Commit(_)).Times(0);
    EXPECT_CALL(*storage_, Revert(_)).Times(0);
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    bool success = false;

    // formulate the query
    query_type query;
    query.MakeObject();
    query["address"] = byte_array::ToBase64(address);

    query_type response;
    if (Contract::Status::OK ==
        contract_->DispatchQuery("balance", query, response))
    {
      balance = response["balance"].As<uint64_t>();
      success = true;
    }

    return success;
  }

  address_type GenerateAddress()
  {
    fetch::byte_array::ByteArray buffer;
    buffer.Resize(IDENTITY_SIZE);

    for (std::size_t i = 0; i < IDENTITY_SIZE; ++i)
    {
      buffer[i] = static_cast<uint8_t>(std::rand() & 0xFF);
    }

    return buffer;
  }

  contract_type contract_;
  storage_type  storage_;
};

TEST_F(TokenContractTests, CheckWealthCreation)
{

  auto address = GenerateAddress();

  // create wealth for this address
  EXPECT_TRUE(CreateWealth(address, 1000));

  // generate the transaction contents
  uint64_t balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, balance));
  EXPECT_EQ(balance, 1000);
}

TEST_F(TokenContractTests, CheckInitialBalance)
{
  auto address = GenerateAddress();

  // generate the transaction contents
  uint64_t balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address, balance));
  EXPECT_EQ(balance, 0);
}

TEST_F(TokenContractTests, CheckTransfer)
{
  auto address1 = GenerateAddress();
  auto address2 = GenerateAddress();

  // create wealth for the first address
  EXPECT_TRUE(CreateWealth(address1, 1000));

  // transfer from wealth
  EXPECT_TRUE(Transfer(address1, address2, 400));

  uint64_t balance = std::numeric_limits<uint64_t>::max();
  EXPECT_TRUE(GetBalance(address1, balance));
  EXPECT_EQ(balance, 600);

  EXPECT_TRUE(GetBalance(address2, balance));
  EXPECT_EQ(balance, 400);
}
