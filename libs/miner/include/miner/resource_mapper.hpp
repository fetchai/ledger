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
#include "ledger/identifier.hpp"
#include "storage/resource_mapper.hpp"

#include <cstdint>
#include <string>

namespace fetch {
namespace miner {

inline uint32_t MapResourceToLane(byte_array::ConstByteArray const &resource,
                                  byte_array::ConstByteArray const &contract,
                                  uint32_t                          log2_num_lanes)
{
  ledger::Identifier identifier(contract);

  return storage::ResourceAddress{
      byte_array::ByteArray{}.Append(identifier.name_space(), ".state.", resource)}
      .lane(log2_num_lanes);
}

}  // namespace miner
}  // namespace fetch
