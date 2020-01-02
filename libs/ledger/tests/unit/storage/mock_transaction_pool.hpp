#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "chain/transaction.hpp"
#include "core/digest.hpp"
#include "ledger/storage_unit/transaction_memory_pool.hpp"

#include "gmock/gmock.h"

class MockTransactionPool : public fetch::ledger::TransactionPoolInterface
{
public:
  using Digest                = fetch::Digest;
  using Transaction           = fetch::chain::Transaction;
  using TransactionMemoryPool = fetch::ledger::TransactionMemoryPool;

  MockTransactionPool()
  {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, Add(_)).WillByDefault(Invoke(&pool, &TransactionMemoryPool::Add));
    ON_CALL(*this, Has(_)).WillByDefault(Invoke(&pool, &TransactionMemoryPool::Has));
    ON_CALL(*this, Get(_, _)).WillByDefault(Invoke(&pool, &TransactionMemoryPool::Get));
    ON_CALL(*this, GetCount()).WillByDefault(Invoke(&pool, &TransactionMemoryPool::GetCount));
    ON_CALL(*this, Remove(_)).WillByDefault(Invoke(&pool, &TransactionMemoryPool::Remove));
  }

  MOCK_METHOD1(Add, void(Transaction const &));
  MOCK_CONST_METHOD1(Has, bool(Digest const &));
  MOCK_CONST_METHOD2(Get, bool(Digest const &, Transaction &));
  MOCK_CONST_METHOD0(GetCount, uint64_t());
  MOCK_METHOD1(Remove, void(Digest const &));

  TransactionMemoryPool pool;
};
