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
#include "ledger/chaincode/contract.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "vm/state_sentinel.hpp"

namespace fetch {
namespace ledger {

class DatabaseInterface : public vm::ReadWriteInterface
{
public:
  static constexpr char const *LOGGING_NAME = "DatabaseInterface";

  using ConstByteArray = byte_array::ConstByteArray;
  using ByteArray      = byte_array::ByteArray;

  DatabaseInterface(SmartContract *context);

  ~DatabaseInterface();

  void Allow(ByteArray const &resource);

  bool write(uint8_t const *const source, uint64_t dest_size, uint8_t const *const keyy,
             uint64_t key_size) override;

  bool read(uint8_t *dest, uint64_t dest_size, uint8_t const *const keyy,
            uint64_t key_size) override;

  bool AccessResource(ByteArray const &key);

  void WriteBackToState();

private:
  SmartContract *                context_;
  std::set<ConstByteArray>       allowed_resources_;
  std::map<ByteArray, ByteArray> cached_resources_;
};

}  // namespace ledger
}  // namespace fetch
