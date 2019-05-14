////------------------------------------------------------------------------------
////
////   Copyright 2018-2019 Fetch.AI Limited
////
////   Licensed under the Apache License, Version 2.0 (the "License");
////   you may not use this file except in compliance with the License.
////   You may obtain a copy of the License at
////
////       http://www.apache.org/licenses/LICENSE-2.0
////
////   Unless required by applicable law or agreed to in writing, software
////   distributed under the License is distributed on an "AS IS" BASIS,
////   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////   See the License for the specific language governing permissions and
////   limitations under the License.
////
////------------------------------------------------------------------------------
//
//#include "core/byte_array/encoders.hpp"
//#include "ledger/chain/mutable_transaction.hpp"
//#include "ledger/chain/transaction.hpp"
//#include "ledger/executor.hpp"
//#include "ledger/chain/v2/transaction_builder.hpp"
//#include "ledger/chain/v2/address.hpp"
//
//#include "mock_storage_unit.hpp"
//
//#include <gtest/gtest.h>
//#include <memory>
//#include <random>
//
//using ::testing::_;
//using ::testing::AtLeast;
//using fetch::ledger::v2::Transaction;
//using fetch::ledger::v2::TransactionBuilder;
//using fetch::ledger::v2::Address;
//using fetch::ledger::Executor;
//using fetch::crypto::ECDSASigner;
//using fetch::crypto::Prover;
//
//class ExecutorTests : public ::testing::Test
//{
//protected:
//  using ExecutorPtr = std::unique_ptr<Executor>;
//  using StoragePtr  = std::shared_ptr<MockStorageUnit>;
//  using ProverPtr   = std::shared_ptr<Prover>;
//
//  void SetUp() override
//  {
//    prover_   = std::make_shared<ECDSASigner>();
//    storage_  = std::make_shared<MockStorageUnit>();
//    executor_ = std::make_unique<Executor>(storage_);
//  }
//
//  std::shared_ptr<Transaction> CreateDummyTransaction()
//  {
//    return TransactionBuilder()
//      .From(Address{prover_->identity()})
//      .TargetChainCode("fetch.dummy", fetch::BitVector{})
//      .Action("wait")
//      .Signer(prover_->identity())
//      .Seal()
//      .Sign(*prover_)
//      .Build();
//  }
//
//  std::shared_ptr<Transaction> CreateWalletTransaction()
//  {
//    std::ostringstream oss;
//    oss << "{ " << R"("amount": )" << 1000 << " }";
//
//    return TransactionBuilder()
//      .From(Address{prover_->identity()})
//      .TargetChainCode("fetch.token", fetch::BitVector{})
//      .Action("wealth")
//      .Data(oss.str())
//      .Signer(prover_->identity())
//      .Seal()
//      .Sign(*prover_)
//      .Build();
//  }
//
//  ProverPtr   prover_;
//  StoragePtr  storage_;
//  ExecutorPtr executor_;
//};
//
//TEST_F(ExecutorTests, CheckDummyContract)
//{
//  EXPECT_CALL(*storage_, AddTransaction(_)).Times(1);
//  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(1);
//  EXPECT_CALL(*storage_, Lock(_)).Times(AtLeast(1));
//  EXPECT_CALL(*storage_, Get(_)).Times(1);
//  EXPECT_CALL(*storage_, Set(_, _)).Times(1);
//  EXPECT_CALL(*storage_, Unlock(_)).Times(AtLeast(1));
//
//  // create the dummy contract
//  auto tx = CreateDummyTransaction();
//
//  // store the transaction inside the store
//  storage_->AddTransaction(*tx);
//
//  fetch::BitVector shards{1};
//  shards.SetAllOne();
//
//  auto const result = executor_->Execute(tx->digest(), 0, 0, shards);
//  EXPECT_EQ(result.status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
//}
//
//TEST_F(ExecutorTests, CheckTokenContract)
//{
//  ::testing::InSequence seq;
//
//  EXPECT_CALL(*storage_, AddTransaction(_)).Times(1);
//  EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(1);
//  EXPECT_CALL(*storage_, Lock(_)).Times(1);
//  EXPECT_CALL(*storage_, Get(_)).Times(1);
//  EXPECT_CALL(*storage_, Set(_, _)).Times(1);
//  EXPECT_CALL(*storage_, Unlock(_)).Times(1);
//
//  // create the dummy contract
//  auto tx = CreateWalletTransaction();
//
//  // store the transaction inside the store
//  storage_->AddTransaction(*tx);
//
//  fetch::BitVector shards{1};
//  shards.SetAllOne();
//
//  auto const result = executor_->Execute(tx->digest(), 0, 0, shards);
//  EXPECT_EQ(result.status, fetch::ledger::ExecutorInterface::Status::SUCCESS);
//}
