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

//#include "ledger/chain/mutable_transaction.hpp"
#include <string>

namespace fetch {
namespace ledger {

class StateWarden
{
public:
  //StateWarden(Transaction const &tx);
  StateWarden(std::string name) {meta = name;};
  ~StateWarden() = default;

  static constexpr char const *LOGGING_NAME = "StateWarden";

  int doIt()
  {
    return 55;
  }

  int (StateWarden::*x)();

private:
  std::string meta{"thingiea"};
  //TransactionSummary::ResourceSet resources_;
};

}  // namespace ledger
}  // namespace fetch
