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

namespace fetch {
namespace ledger {

class TokenContract : public Contract
{
public:
  static constexpr char const *LOGGING_NAME = "TokenContract";
  static constexpr char const *NAME         = "fetch.token";

  // Construction / Destruction
  TokenContract();
  ~TokenContract() override = default;

private:
  // transaction handlers
  Status CreateWealth(Transaction const &tx);
  Status Deed(Transaction const &tx);
  Status Transfer(Transaction const &tx);

  // queries
  Status Balance(Query const &query, Query &response);
};

}  // namespace ledger
}  // namespace fetch
