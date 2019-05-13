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

#include "ledger/chaincode/contract.hpp"
#include "ledger/chain/v2/transaction.hpp"

namespace fetch {
namespace ledger {

namespace v2 {
  class Address;
  class Transaction;
}

class TokenContract : public Contract
{
public:
  static constexpr char const *LOGGING_NAME = "TokenContract";
  static constexpr char const *NAME         = "fetch.token";

  // Construction / Destruction
  TokenContract();
  ~TokenContract() override = default;

  // library functions
  uint64_t GetBalance(v2::Address const &address);
  bool AddTokens(v2::Address const &address, uint64_t amount);
  bool SubtractTokens(v2::Address const &address, uint64_t amount);
  bool TransferTokens(v2::Transaction const &tx, v2::Address const &to, uint64_t amount);

  // transaction handlers
  Status CreateWealth(v2::Transaction const &tx);
  Status Deed(v2::Transaction const &tx);
  Status Transfer(v2::Transaction const &tx);

  // queries
  Status Balance(Query const &query, Query &response);
};

}  // namespace ledger
}  // namespace fetch
