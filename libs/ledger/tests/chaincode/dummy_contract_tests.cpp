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

#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/dummy_contract.hpp"
#include "mock_storage_unit.hpp"

#include <gmock/gmock.h>

#include <iostream>
#include <memory>

using ::testing::_;
using namespace fetch;
using namespace fetch::ledger;

class DummyContractTests : public ::testing::Test
{
protected:
  using contract_type = std::unique_ptr<DummyContract>;
  using storage_type  = std::unique_ptr<MockStorageUnit>;

  void SetUp() override
  {
    contract_ = std::make_unique<DummyContract>();
    storage_  = std::make_unique<MockStorageUnit>();

    contract_->Attach(*storage_);
  }

  contract_type contract_;
  storage_type  storage_;
};

TEST_F(DummyContractTests, CheckConstruction)
{

  // we expect that the state database is not called this time
  EXPECT_CALL(*storage_, Get(_)).Times(0);
  EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
  EXPECT_CALL(*storage_, Set(_, _)).Times(0);
  EXPECT_CALL(*storage_, Lock(_)).Times(0);
  EXPECT_CALL(*storage_, Unlock(_)).Times(0);
  EXPECT_CALL(*storage_, Hash()).Times(0);
  EXPECT_CALL(*storage_, Commit(_)).Times(0);
  EXPECT_CALL(*storage_, Revert(_)).Times(0);
  EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);
}

TEST_F(DummyContractTests, CheckDispatch)
{

  // since the dummy contract doesn't use the state database we expect no calls
  // to it
  EXPECT_CALL(*storage_, Get(_)).Times(0);
  EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
  EXPECT_CALL(*storage_, Set(_, _)).Times(0);
  EXPECT_CALL(*storage_, Lock(_)).Times(0);
  EXPECT_CALL(*storage_, Unlock(_)).Times(0);
  EXPECT_CALL(*storage_, Hash()).Times(0);
  EXPECT_CALL(*storage_, Commit(_)).Times(0);
  EXPECT_CALL(*storage_, Revert(_)).Times(0);
  EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

  // create the sample transaction
  chain::MutableTransaction tx;
  tx.set_contract_name("fetch.dummy.wait");

  Identifier identifier;
  identifier.Parse(tx.contract_name());

  contract_->DispatchTransaction(identifier.name(), chain::VerifiedTransaction::Create(tx));
  EXPECT_EQ(contract_->GetTransactionCounter("wait"), 1u);
}
