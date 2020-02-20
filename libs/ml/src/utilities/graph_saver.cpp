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

#include "core/byte_array/decoders.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "core/serialisers/main_serialiser.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "ml/utilities/graph_saver.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace fetch {
namespace ml {
namespace utilities {

template <typename GraphType>
void SaveGraph(GraphType &g, std::string const &save_location)
{
  using TensorType = typename GraphType::TensorType;

  std::cout << "Starting large graph serialization" << std::endl;

  fetch::ml::GraphSaveableParams<TensorType> gsp = g.GetGraphSaveableParams();

  fetch::serializers::LargeObjectSerializeHelper serializer;

  serializer << gsp;

  std::ofstream outFile(save_location, std::ios::out | std::ios::binary);

  if (outFile)
  {
    outFile.write(serializer.data().char_pointer(), std::streamsize(serializer.size()));
    outFile.close();
    std::cout << "Buffer size " << serializer.size() << std::endl;
    std::cout << "Finish writing to file " << save_location << std::endl;
  }
  else
  {
    std::cerr << "Can't open save file" << std::endl;
  }
}

template <typename GraphType>
std::shared_ptr<GraphType> LoadGraph(std::string const &save_location)
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

template void SaveGraph<fetch::ml::Graph<math::Tensor<int8_t>>>(
    fetch::ml::Graph<math::Tensor<int8_t>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<int16_t>>>(
    fetch::ml::Graph<math::Tensor<int16_t>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<int32_t>>>(
    fetch::ml::Graph<math::Tensor<int32_t>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<int64_t>>>(
    fetch::ml::Graph<math::Tensor<int64_t>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<float>>>(
    fetch::ml::Graph<math::Tensor<float>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<double>>>(
    fetch::ml::Graph<math::Tensor<double>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> &g, std::string const &save_location);
template void SaveGraph<fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> &g, std::string const &save_location);

template std::shared_ptr<fetch::ml::Graph<math::Tensor<int8_t>>>
LoadGraph<fetch::ml::Graph<math::Tensor<int8_t>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<int16_t>>>
LoadGraph<fetch::ml::Graph<math::Tensor<int16_t>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<int32_t>>>
LoadGraph<fetch::ml::Graph<math::Tensor<int32_t>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<int64_t>>>
LoadGraph<fetch::ml::Graph<math::Tensor<int64_t>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<float>>>
LoadGraph<fetch::ml::Graph<math::Tensor<float>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<double>>>
LoadGraph<fetch::ml::Graph<math::Tensor<double>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>>>
LoadGraph<fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>>>
LoadGraph<fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>>>(std::string const &save_location);
template std::shared_ptr<fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>>>
LoadGraph<fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>>>(std::string const &save_location);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
