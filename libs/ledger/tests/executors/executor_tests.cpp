#include "core/byte_array/encoders.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/executor.hpp"

#include "mock_storage_unit.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <random>

using ::testing::_;

class ExecutorTests : public ::testing::Test
{
protected:
  using underlying_executor_type = fetch::ledger::Executor;
  using executor_type            = std::unique_ptr<underlying_executor_type>;
  using underlying_storage_type  = MockStorageUnit;
  using storage_type             = std::shared_ptr<underlying_storage_type>;
  using rng_type                 = std::mt19937;

  static constexpr std::size_t IDENTITY_SIZE = 64;

  ExecutorTests() { rng_.seed(42); }

  void SetUp() override
  {
    storage_.reset(new underlying_storage_type);
    executor_.reset(new underlying_executor_type(storage_));
  }

  fetch::chain::Transaction CreateDummyTransaction()
  {
    fetch::chain::MutableTransaction tx;
    tx.set_contract_name("fetch.dummy.wait");
    return fetch::chain::VerifiedTransaction::Create(std::move(tx));
  }

  fetch::byte_array::ConstByteArray CreateAddress()
  {
    fetch::byte_array::ByteArray address;
    address.Resize(std::size_t{IDENTITY_SIZE});

    for (std::size_t i = 0; i < std::size_t{IDENTITY_SIZE}; ++i)
    {
      address[i] = static_cast<uint8_t>(rng_() & 0xFF);
    }

    return {address};
  }

  fetch::chain::Transaction CreateWalletTransaction()
  {

    // generate an address
    auto address = CreateAddress();

    // format the transaction contents
    std::ostringstream oss;
    oss << "{ "
        << R"("address": ")" << static_cast<std::string>(fetch::byte_array::ToBase64(address))
        << "\", "
        << R"("amount": )" << 1000 << " }";

    // create the transaction
    fetch::chain::MutableTransaction tx;
    tx.set_contract_name("fetch.token.wealth");
    tx.set_data(oss.str());

    return fetch::chain::VerifiedTransaction::Create(std::move(tx));
  }

  storage_type  storage_;
  executor_type executor_;
  rng_type      rng_;
};

TEST_F(ExecutorTests, CheckDummyContract)
{

  EXPECT_CALL(*storage_, AddTransaction(_)).Times(1);
  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(1);

  // create the dummy contract
  auto tx = CreateDummyTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  executor_->Execute(tx.digest(), 0, {0});
}

TEST_F(ExecutorTests, CheckTokenContract)
{

  EXPECT_CALL(*storage_, AddTransaction(_)).Times(1);
  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(1);
  EXPECT_CALL(*storage_, GetOrCreate(_)).Times(1);
  EXPECT_CALL(*storage_, Set(_, _)).Times(1);

  // create the dummy contract
  auto tx = CreateWalletTransaction();

  // store the transaction inside the store
  storage_->AddTransaction(tx);

  executor_->Execute(tx.digest(), 0, {0});
}
