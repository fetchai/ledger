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

#include "core/serializers/byte_array_buffer.hpp"
#include "ml/serializers/ml_types.hpp"

namespace fetch {
namespace ml {
namespace examples {

/**
 * Saves the state dict of a graph to a file location specified by user
 * @param g the graph to save
 * @param save_location a string specifying save location
 */
template <typename GraphType>
void SaveModel(GraphType const &g, std::string const &save_location)
{
  fetch::serializers::MsgPackSerializer serializer;
  serializer << g.StateDict();

  std::fstream file(save_location, std::fstream::out);  // fba = FetchByteArray
  if (file)
  {
    file << std::string(serializer.data());
    file.close();
  }
  else
  {
    std::cerr << "Can't open save file" << std::endl;
  }
}

}  // namespace examples
}  // namespace ml
}  // namespace fetch
