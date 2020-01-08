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

#include "core/filesystem/write_to_file.hpp"

#include <fstream>

namespace fetch {
namespace core {

bool WriteToFile(char const *filename, byte_array::ConstByteArray const &data)
{
  bool          success{false};
  std::ofstream stream{filename, std::ios::out | std::ios::binary | std::ios::trunc};
  if (stream.is_open())
  {
    stream.write(data.char_pointer(), static_cast<std::streamsize>(data.size()));

    success = (!(stream.bad() || stream.fail()));
  }

  return success;
}

}  // namespace core
}  // namespace fetch
