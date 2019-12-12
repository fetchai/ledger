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

#include "ledger/execution_result.hpp"

namespace fetch {
namespace chain {

class Transaction;

}  // namespace chain

namespace ledger {

class Block;
class StorageInterface;
class TokenContract;

/**
 * Functor like object that can perform validation checks to determine if a transaction is valid to
 * be included into a specified block.
 *
 * This object is designed to be used not only in the execution phase of a node but also as a filter
 * for miners
 */
class TransactionValidator
{
public:
  // Construction / Destruction
  TransactionValidator(StorageInterface &storage, TokenContract &token_contract);
  TransactionValidator(TransactionValidator const &) = delete;
  TransactionValidator(TransactionValidator &&)      = delete;
  ~TransactionValidator()                            = default;

  ContractExecutionStatus operator()(chain::Transaction const &tx, uint64_t block_index) const;

  // Operators
  TransactionValidator &operator=(TransactionValidator const &) = delete;
  TransactionValidator &operator=(TransactionValidator &&) = delete;

private:
  StorageInterface &storage_;
  TokenContract &   token_contract_;
};

}  // namespace ledger
}  // namespace fetch
