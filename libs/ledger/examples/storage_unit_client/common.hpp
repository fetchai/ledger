#pragma once
///------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
namespace examples {
namespace common {

template <typename T>
void ToBase64(T &stream, chain::Transaction::signatures_type::value_type const &signature)
{
  stream << "signature: " << (signature.second.signature_data.ToBase64())
         << ", sig.type: " << (signature.second.type.ToBase64())
         << ", identity: " << (signature.first.identifier().ToBase64())
         << ", ident.params: " << (signature.first.parameters().ToBase64()) << std::endl;
}

template <typename T>
void ToBase64(T &stream, chain::Transaction::signatures_type const &signatures)
{
  for (auto const &sig : signatures)
  {
    ToBase64(stream, sig);
  }
}

}  // namespace common
}  // namespace examples
}  // namespace ledger
}  // namespace fetch
