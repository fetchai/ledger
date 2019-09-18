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

#include "core/serializers/main_serializer.hpp"
#include "ml/serializers/ml_types.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace fetch {
namespace ml {
namespace examples {

/**
 * Saves the state dict of a graph to a file location specified by user
 * @param g the graph to save
 * @param save_location a string specifying save location
 */
template <typename GraphType>
void SaveModel(GraphType &g, std::string const &save_location)
{
  using TensorType = typename GraphType::TensorType;

  std::cout << "Starting graph serialization" << std::endl;

  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g.GetGraphSaveableParams();

  fetch::serializers::MsgPackSerializer serializer;

  serializer << gsp1;

  std::fstream file(save_location, std::fstream::out);  // fba = FetchByteArray

  if (file)
  {
    file << std::string(serializer.data());
    file.close();
    std::cout << "Finish writing to file " << save_location << std::endl;
  }
  else
  {
    std::cerr << "Can't open save file" << std::endl;
  }
}

template <typename GraphType>
void SaveLargeModel(GraphType &g, std::string const &save_location)
{
  using TensorType = typename GraphType::TensorType;

  std::cout << "Starting large graph serialization" << std::endl;

  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g.GetGraphSaveableParams();

  fetch::serializers::LargeObjectSerializeHelper losh;

  losh << gsp1;

  std::ofstream outFile(save_location, std::ios::out | std::ios::binary);

  if (outFile)
  {
    outFile.write(losh.buffer.data().char_pointer(), std::streamsize(losh.buffer.size()));
    outFile.close();
    std::cout << "Buffer size " << losh.buffer.size() << std::endl;
    std::cout << "Finish writing to file " << save_location << std::endl;
  }
  else
  {
    std::cerr << "Can't open save file" << std::endl;
  }
}

}  // namespace examples
}  // namespace ml
}  // namespace fetch
