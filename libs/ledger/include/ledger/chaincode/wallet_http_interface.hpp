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

#include "core/byte_array/const_byte_array.hpp"
#include "http/module.hpp"
#include "storage/object_store.hpp"

#include <cstdint>

namespace fetch {
namespace ledger {

class StorageInterface;
class TransactionProcessor;

class WalletHttpInterface : public http::HTTPModule
{
public:
  static constexpr char const *LOGGING_NAME = "WalletHttpInterface";

  enum class ErrorCode
  {
    NOT_IMPLEMENTED = 1000,
    PARSE_FAILURE
  };

  // Construction / Destruction
  WalletHttpInterface(StorageInterface &state, TransactionProcessor &processor,
                      std::size_t num_lanes);
  WalletHttpInterface(WalletHttpInterface const &) = delete;
  WalletHttpInterface(WalletHttpInterface &&)      = delete;
  ~WalletHttpInterface()                           = default;

  // Operators
  WalletHttpInterface &operator=(WalletHttpInterface const &) = delete;
  WalletHttpInterface &operator=(WalletHttpInterface &&) = delete;

private:
  using KeyStore = storage::ObjectStore<byte_array::ConstByteArray>;

  http::HTTPResponse OnRegister(http::HTTPRequest const &request);
  http::HTTPResponse OnBalance(http::HTTPRequest const &request);
  http::HTTPResponse OnTransfer(http::HTTPRequest const &request);
  http::HTTPResponse OnTransactions(http::HTTPRequest const &request);

  static http::HTTPResponse BadJsonResponse(ErrorCode error_code);
  static const char *       ToString(ErrorCode error_code);

  StorageInterface &    state_;
  TransactionProcessor &processor_;
  KeyStore              key_store_;
  std::size_t           num_lanes_{0};
  uint32_t              log2_lanes_{0};
};

}  // namespace ledger
}  // namespace fetch
