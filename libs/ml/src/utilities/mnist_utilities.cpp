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
#include <ml/utilities/mnist_utilities.hpp>
#include "math/tensor/tensor.hpp"

#include "math/one_hot.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "vectorise/platform.hpp"


#include <fstream>
#include <string>

namespace fetch {
namespace ml {
namespace utilities {

template <typename TensorType>
TensorType read_mnist_images(std::string const &full_path)
{
  using DataType = typename TensorType::Type;

  std::ifstream file(full_path, std::ios::binary);

  if (!file.is_open())
  {
    throw exceptions::InvalidFile("Cannot open file `" + full_path + "`!");
  }

  uint32_t magic_number = 0, n_rows = 0, n_cols = 0;

  file.read(reinterpret_cast<char *>(&magic_number), sizeof(magic_number));
  magic_number = platform::ToBigEndian(magic_number);

  if (magic_number != 2051)
  {
    throw exceptions::InvalidFile("Invalid MNIST image file!");
  }

  uint32_t n_images;
  file.read(reinterpret_cast<char *>(&n_images), sizeof(n_images));
  n_images = platform::ToBigEndian(n_images);

  file.read(reinterpret_cast<char *>(&n_rows), sizeof(n_rows));
  n_rows = platform::ToBigEndian(n_rows);
  file.read(reinterpret_cast<char *>(&n_cols), sizeof(n_cols));
  n_cols = platform::ToBigEndian(n_cols);

  SizeType image_size{n_rows * n_cols};

  TensorType tensor_dataset{{n_rows, n_cols, n_images}};
  auto       image_char = new char[image_size];

  for (SizeType i{0}; i < SizeType{n_images}; i++)
  {
    file.read(image_char, std::streamsize(image_size));
    for (SizeType j{0}; j < n_rows; j++)
    {
      for (SizeType k{0}; k < n_cols; k++)
      {
        tensor_dataset.At(j, k, i) =
            static_cast<DataType>(static_cast<uint8_t>(image_char[j * n_cols + k])) / DataType{256};
      }
    }
  }

  return tensor_dataset;
};

template <typename TensorType>
TensorType read_mnist_labels(std::string const &full_path)
{
  using DataType = typename TensorType::Type;
  std::ifstream file(full_path, std::ios::binary);

  if (!file.is_open())
  {
    throw exceptions::InvalidFile("Cannot open file `" + full_path + "`!");
  }

  uint32_t magic_number = 0;

  file.read(reinterpret_cast<char *>(&magic_number), sizeof(magic_number));
  magic_number = platform::ToBigEndian(magic_number);

  if (magic_number != 2049)
  {
    throw exceptions::InvalidFile("Invalid MNIST label file!");
  }

  uint32_t n_labels;
  file.read(reinterpret_cast<char *>(&n_labels), sizeof(n_labels));
  n_labels = platform::ToBigEndian(n_labels);

  TensorType labels{{1, n_labels}};
  char       label_char;
  for (SizeType i{0}; i < SizeType{n_labels}; i++)
  {
    file.read(&label_char, 1);
    labels.At(0, i) = static_cast<DataType>(static_cast<uint8_t>(label_char));
  }

  return labels;
}

template <typename TensorType>
TensorType convert_labels_to_onehot(TensorType labels)
{
  assert(labels.shape().at(0) == 1);
  SizeType n_labels{10};

  // one_hot has shape {10, 1, batch_size}
  TensorType one_hot = fetch::math::OneHot(labels, n_labels);

  return one_hot.View().Copy();
}


template math::Tensor<int32_t> read_mnist_images<math::Tensor<int32_t>>(std::string const &full_path);
template math::Tensor<int64_t> read_mnist_images<math::Tensor<int64_t>>(std::string const &full_path);
template math::Tensor<float> read_mnist_images<math::Tensor<float>>(std::string const &full_path);
template math::Tensor<double> read_mnist_images<math::Tensor<double>>(std::string const &full_path);
template math::Tensor<fixed_point::fp32_t> read_mnist_images<math::Tensor<fixed_point::fp32_t>>(std::string const &full_path);
template math::Tensor<fixed_point::fp64_t> read_mnist_images<math::Tensor<fixed_point::fp64_t>>(std::string const &full_path);
template math::Tensor<fixed_point::fp128_t> read_mnist_images<math::Tensor<fixed_point::fp128_t>>(std::string const &full_path);

		template math::Tensor<int32_t> read_mnist_labels<math::Tensor<int32_t>>(std::string const &full_path);
		template math::Tensor<int64_t> read_mnist_labels<math::Tensor<int64_t>>(std::string const &full_path);
		template math::Tensor<float> read_mnist_labels<math::Tensor<float>>(std::string const &full_path);
		template math::Tensor<double> read_mnist_labels<math::Tensor<double>>(std::string const &full_path);
		template math::Tensor<fixed_point::fp32_t> read_mnist_labels<math::Tensor<fixed_point::fp32_t>>(std::string const &full_path);
		template math::Tensor<fixed_point::fp64_t> read_mnist_labels<math::Tensor<fixed_point::fp64_t>>(std::string const &full_path);
		template math::Tensor<fixed_point::fp128_t> read_mnist_labels<math::Tensor<fixed_point::fp128_t>>(std::string const &full_path);

template math::Tensor<int32_t> convert_labels_to_onehot<math::Tensor<int32_t>>(math::Tensor<int32_t> labels);
template math::Tensor<int64_t> convert_labels_to_onehot<math::Tensor<int64_t>>(math::Tensor<int64_t> labels);
template math::Tensor<float> convert_labels_to_onehot<math::Tensor<float>>(math::Tensor<float> labels);
template math::Tensor<double> convert_labels_to_onehot<math::Tensor<double>>(math::Tensor<double> labels);
template math::Tensor<fixed_point::fp32_t> convert_labels_to_onehot<math::Tensor<fixed_point::fp32_t>>(math::Tensor<fixed_point::fp32_t> labels);
template math::Tensor<fixed_point::fp64_t> convert_labels_to_onehot<math::Tensor<fixed_point::fp64_t>>(math::Tensor<fixed_point::fp64_t> labels);
template math::Tensor<fixed_point::fp128_t> convert_labels_to_onehot<math::Tensor<fixed_point::fp128_t>>(math::Tensor<fixed_point::fp128_t> labels);
}
}
}



