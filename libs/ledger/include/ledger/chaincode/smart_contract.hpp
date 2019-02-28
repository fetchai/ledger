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

class SmartContract : public Contract
{
public:
  SmartContract(byte_array::ConstByteArray const &identifier);
  ~SmartContract() = default;

  static constexpr char const *LOGGING_NAME = "SmartContract";

  bool Exists(byte_array::ByteArray const &address);
  bool Get(byte_array::ByteArray &record, byte_array::ByteArray const &address);
  void Set(byte_array::ByteArray const &record, byte_array::ByteArray const &address);

  bool SetupHandlers() override;

private:
  // transaction handlers
  Status InvokeContract(Transaction const &tx);

  bool RunSmartContract(std::string &source, byte_array::ConstByteArray const &target_fn,
                        byte_array::ConstByteArray const &hash, Transaction const &tx);

  std::string source_;
};

}  // namespace ledger
}  // namespace fetch
