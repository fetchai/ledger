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

#include "http/module.hpp"

namespace fetch {
namespace ledger {

class TransactionStatusCache;

class TxStatusHttpInterface : public http::HTTPModule
{
public:
  // Construction / Destruction
  explicit TxStatusHttpInterface(TransactionStatusCache &status_cache);
  TxStatusHttpInterface(TxStatusHttpInterface const &) = delete;
  TxStatusHttpInterface(TxStatusHttpInterface &&)      = delete;
  ~TxStatusHttpInterface()                             = default;

  // Operators
  TxStatusHttpInterface &operator=(TxStatusHttpInterface const &) = delete;
  TxStatusHttpInterface &operator=(TxStatusHttpInterface &&) = delete;

private:
  TransactionStatusCache &status_cache_;
};

}  // namespace ledger
}  // namespace fetch
