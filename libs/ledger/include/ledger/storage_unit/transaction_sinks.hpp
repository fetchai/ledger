#pragma once
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

#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace ledger {

class UnverifiedTransactionSink
{
public:
  using TransactionList = std::vector<UnverifiedTransaction>;

  // Construction / Destruction
  UnverifiedTransactionSink()          = default;
  virtual ~UnverifiedTransactionSink() = default;

  /// @name Transaction Handlers
  /// @{
  virtual void OnTransaction(UnverifiedTransaction const &tx) = 0;
  /// @}
};

class VerifiedTransactionSink
{
public:
  using TransactionList = std::vector<VerifiedTransaction>;

  // Construction / Destruction
  VerifiedTransactionSink()          = default;
  virtual ~VerifiedTransactionSink() = default;

  /// @name Transaction Handlers
  /// @{
  virtual void OnTransaction(VerifiedTransaction const &tx) = 0;
  virtual void OnTransactions(TransactionList const &txs);
  /// @}
};

inline void VerifiedTransactionSink::OnTransactions(TransactionList const &txs)
{
  for (auto const &tx : txs)
  {
    OnTransaction(tx);
  }
}

}  // namespace ledger
}  // namespace fetch
