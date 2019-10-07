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

#include "core/byte_array/decoders.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace fetch {
namespace ml {
namespace examples {

/**
 * Saves the saveparams of a graph to a file location specified by user
 * @param g the graph to save
 * @param save_location a string specifying save location
 */
template <typename GraphType>
void SaveModel(GraphType &g, std::string const &save_location)
{
  using TensorType = typename GraphType::TensorType;

  std::cout << "Starting large graph serialization" << std::endl;

  fetch::ml::GraphSaveableParams<TensorType> gsp = g.GetGraphSaveableParams();

  fetch::serializers::LargeObjectSerializeHelper serializer;

  serializer << gsp;

  std::ofstream outFile(save_location, std::ios::out | std::ios::binary);

  if (outFile)
  {
    outFile.write(serializer.buffer.data().char_pointer(),
                  std::streamsize(serializer.buffer.size()));
    outFile.close();
    std::cout << "Buffer size " << serializer.buffer.size() << std::endl;
    std::cout << "Finish writing to file " << save_location << std::endl;
  }
  else
  {
    std::cerr << "Can't open save file" << std::endl;
  }
}

template <typename GraphType>
std::shared_ptr<GraphType> LoadModel(std::string const &save_location)
{
  using TensorType = typename GraphType::TensorType;
  std::cout << "Loading graph" << std::endl;

  fetch::byte_array::ConstByteArray buffer = fetch::core::ReadContentsOfFile(save_location.c_str());
  if (buffer.empty())
  {
    throw exceptions::InvalidFile("File does not exist");
  }

  fetch::serializers::LargeObjectSerializeHelper serializer(buffer);

  fetch::ml::GraphSaveableParams<TensorType> gsp;

  serializer >> gsp;

  auto graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();

  fetch::ml::utilities::BuildGraph<TensorType>(gsp, graph_ptr);

  return graph_ptr;
}

}  // namespace examples
}  // namespace ml
}  // namespace fetch
