#include "core/make_unique.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/dummy_contract.hpp"
#include "mock_state_database.hpp"

#include <gmock/gmock.h>

#include <memory>
#include <iostream>

using ::testing::_;
using namespace fetch;
using namespace fetch::ledger;

class DummyContractTests : public ::testing::Test {
protected:
  using contract_type = std::unique_ptr<DummyContract>; // TODO: EJF Rename this class
  using storage_type = std::unique_ptr<MockStateDatabase>;

  void SetUp() override {
    contract_ = fetch::make_unique<DummyContract>();
    storage_ = fetch::make_unique<MockStateDatabase>();

    contract_->Attach(*storage_);
  }

  contract_type contract_;
  storage_type storage_;
};

TEST_F(DummyContractTests, CheckConstruction) {

  // we expect that the state database is not called this time
  EXPECT_CALL(*storage_, GetOrCreate(_))
    .Times(0);
  EXPECT_CALL(*storage_, Get(_))
    .Times(0);
  EXPECT_CALL(*storage_, Set(_, _))
    .Times(0);
  EXPECT_CALL(*storage_, Commit(_))
    .Times(0);
  EXPECT_CALL(*storage_, Revert(_))
    .Times(0);
}

TEST_F(DummyContractTests, CheckDispatch) {

  // since the dummy contract doesn't use the state database we expect no calls to it
  EXPECT_CALL(*storage_, GetOrCreate(_))
    .Times(0);
  EXPECT_CALL(*storage_, Get(_))
    .Times(0);
  EXPECT_CALL(*storage_, Set(_, _))
    .Times(0);
  EXPECT_CALL(*storage_, Commit(_))
    .Times(0);
  EXPECT_CALL(*storage_, Revert(_))
    .Times(0);

  // create the sample transaction
  chain::Transaction tx;
  tx.set_contract_name("fetch.dummy.wait");

  contract_->DispatchTransaction(tx.contract_name().name(), tx);
  EXPECT_EQ(contract_->GetTransactionCounter("wait"), 1u);
}
