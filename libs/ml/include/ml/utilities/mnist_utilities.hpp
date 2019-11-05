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

namespace fetch {
namespace ml {
namespace utilities {

using SizeType = fetch::math::SizeType;

namespace {
auto reverseInt = [](uint32_t i) -> uint32_t {
  // TODO(issue 1674): Change to use platform tools
  uint8_t c1, c2, c3, c4;
  c1 = static_cast<uint8_t>(i & 255u);
  c2 = static_cast<uint8_t>((i >> 8u) & 255u);
  c3 = static_cast<uint8_t>((i >> 16u) & 255u);
  c4 = static_cast<uint8_t>((i >> 24u) & 255u);
  return static_cast<uint32_t>((static_cast<uint32_t>(c1) << 24u) +
                               (static_cast<uint32_t>(c2) << 16u) +
                               (static_cast<uint32_t>(c3) << 8u) + c4);
};
}

// function to read mnist data into a tensor
template <typename TensorType>
TensorType ReadMnistImages(std::string const &full_path)
{
  using DataType = typename TensorType::Type;

  std::ifstream file(full_path, std::ios::binary);

  if (!file.is_open())
  {
    throw exceptions::InvalidFile("Cannot open file `" + full_path + "`!");
  }

  unsigned int magic_number = 0, n_rows = 0, n_cols = 0;

  file.read(reinterpret_cast<char *>(&magic_number), sizeof(magic_number));
  magic_number = reverseInt(magic_number);

  if (magic_number != 2051)
  {
    throw exceptions::InvalidFile("Invalid MNIST image file!");
  }

  uint32_t n_images;
  file.read(reinterpret_cast<char *>(&n_images), sizeof(n_images));
  SizeType number_of_images = reverseInt(n_images);

  file.read(reinterpret_cast<char *>(&n_rows), sizeof(n_rows));
  n_rows = reverseInt(n_rows);
  file.read(reinterpret_cast<char *>(&n_cols), sizeof(n_cols));
  n_cols = reverseInt(n_cols);

  SizeType image_size = n_rows * n_cols;

  TensorType tensor_dataset {{n_rows, n_cols, number_of_images}};

  for (SizeType i{0}; i < number_of_images; i++)
  {
    auto image_char = new char[image_size];
    file.read(image_char, std::streamsize(image_size));
    for (SizeType j{0}; j< n_rows; j++){
      for (SizeType k{0}; k< n_cols; k++) {
        tensor_dataset.At(j, k, i) = static_cast<DataType>(
                                      static_cast<uint8_t >(image_char[j * n_cols + k])) / DataType{256};
      }
    }
  }

  return tensor_dataset;
}

// function to read mnist labels into a tensor
template <typename TensorType>
TensorType ReadMnistLabels(std::string const &full_path)
{
  using DataType = typename TensorType::Type;
  std::ifstream file(full_path, std::ios::binary);

  if (!file.is_open())
  {
    throw exceptions::InvalidFile("Cannot open file `" + full_path + "`!");
  }

  u_int32_t magic_number = 0;

  file.read(reinterpret_cast<char *>(&magic_number), sizeof(magic_number));
  magic_number = reverseInt(magic_number);

  if (magic_number != 2049)
  {
    throw exceptions::InvalidFile("Invalid MNIST label file!");
  }

  uint32_t n_labels;
  file.read(reinterpret_cast<char *>(&n_labels), sizeof(n_labels));
  SizeType number_of_labels = reverseInt(n_labels);

  TensorType labels{{1, number_of_labels}};
  for (SizeType i{0}; i < number_of_labels; i++)
  {
    char label_char;
    file.read(&label_char, 1);
    labels.At(0, i) = static_cast<DataType>(static_cast<uint8_t>(label_char));
  }
  std::cout << "labels.ToString(): " << labels.ToString() << std::endl;
  return labels;
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
