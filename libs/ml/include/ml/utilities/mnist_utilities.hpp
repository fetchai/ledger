#pragma once
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

#include "math/base_types.hpp"

namespace fetch {
namespace ml {
namespace utilities {

using SizeType = fetch::math::SizeType;

/**
 * Reads the mnist image file into a Tensor
 * @param full_path Path to image file
 * @return Tensor with shape {28, 28, n_images}
 */
template <typename TensorType>
TensorType read_mnist_images(std::string const &full_path);

/**
 * Reads the mnist labels file into a Tensor
 * @param full_path Path to labels file
 * @return Tensor with shape {1, n_images}
 */
template <typename TensorType>
TensorType read_mnist_labels(std::string const &full_path);

template <typename TensorType>
TensorType convert_labels_to_onehot(TensorType labels);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
