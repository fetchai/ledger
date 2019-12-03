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
#include "ledger/resource_mapper.hpp"
#include "storage/resource_mapper.hpp"

#include <cstdint>
#include <string>

namespace fetch {
namespace ledger {

namespace {

constexpr char SEPARATOR = '.';

byte_array::ConstByteArray GetNameSpace(byte_array::ConstByteArray const &name)
{
  std::size_t offset = 0;
  std::size_t last_token_size = 0;
  for (;;) {
    // find the next instance of the separator
    std::size_t const index = name.Find(SEPARATOR, offset);

    // determine if this is the last token
    if (byte_array::ConstByteArray::NPOS == index) {
      last_token_size = name.size() - offset + 1;
      break;
    }
    // update the index
    offset = index + 1;
  }
  return name.SubArray(0, name.size() - last_token_size);
}

} //namespace

uint32_t MapResourceToLane(byte_array::ConstByteArray const &resource,
                           byte_array::ConstByteArray const &contract, uint32_t log2_num_lanes)
{
  return storage::ResourceAddress{
      byte_array::ByteArray{}.Append(GetNameSpace(contract), ".state.", resource)}
      .lane(log2_num_lanes);
}

}  // namespace ledger
}  // namespace fetch
