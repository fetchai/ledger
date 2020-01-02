//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/byte_array.hpp"
#include "core/filesystem/read_file_contents.hpp"

#include <cstddef>
#include <fstream>

namespace fetch {
namespace core {

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;

ConstByteArray ReadContentsOfFile(char const *filename, std::streamsize const buffer_size_limit)
{
  ByteArray buffer;

  std::ifstream stream(filename, std::ios::in | std::ios::binary | std::ios::ate);

  if (stream.is_open())
  {
    // determine the complete size of the file
    std::streamsize const stream_size = stream.tellg();
    if (stream_size > 0)
    {
      std::streamsize const buffer_size =
          (buffer_size_limit < 0) ? stream_size : std::min(stream_size, buffer_size_limit);
      stream.seekg(0, std::ios::beg);

      // allocate the buffer
      buffer.Resize(static_cast<std::size_t const &>(buffer_size));
      static_assert(sizeof(stream_size) >= sizeof(std::size_t), "Must be same size or larger");
      // read in size of the buffer
      stream.read(buffer.char_pointer(), buffer_size);
    }
  }

  return {std::move(buffer)};
}

}  // namespace core
}  // namespace fetch
