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

#include "ml/dataloaders/dataloader.hpp"

#include <exception>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

namespace fetch {
namespace ml {

template <typename T>
class MNISTLoader : public DataLoader<uint64_t, T>
{
public:
  MNISTLoader(std::string const &imagesFile, std::string const &labelsFile)
    : cursor_(0)
  {
    std::uint32_t recordLength(0);
    data_   = read_mnist_images(imagesFile, size_, recordLength);
    labels_ = read_mnist_labels(labelsFile, size_);
    assert(recordLength == 28 * 28);
  }

  virtual uint64_t Size() const
  {
    // MNIST files store the size as uint32_t but Dataloader interface require uint64_t
    return static_cast<uint64_t>(size_);
  }

  virtual bool IsDone() const
  {
    return cursor_ >= size_;
  }

  virtual void Reset()
  {
    cursor_ = 0;
  }

  std::pair<uint64_t, T> GetAtIndex(uint64_t index) const
  {
    T buffer({28u, 28u});
    for (std::uint64_t i(0); i < 28 * 28; ++i)
    {
      buffer.At(i) = typename T::Type(data_[index][i]) / typename T::Type(256);
    }
    uint64_t label = (uint64_t)(labels_[index]);
    return std::make_pair(label, buffer);
  }

  virtual std::pair<uint64_t, T> GetNext()
  {
    return GetAtIndex(cursor_++);
  }

  std::pair<uint64_t, T> GetRandom()
  {
    return GetAtIndex((uint64_t)rand() % Size());
  }

  void Display(T const &data) const
  {
    for (std::uint64_t j(0); j < 784; ++j)
    {
      std::cout << (data.At(j) > typename T::Type(0.5) ? char(219) : ' ')
                << ((j % 28 == 0) ? "\n" : "");
    }
    std::cout << std::endl;
  }

  std::pair<T, T> SubsetToArray(uint64_t subset_size)
  {
    T ret_labels({subset_size});
    T ret_images({subset_size, 28 * 28});

    for (std::uint64_t i(0); i < subset_size; ++i)
    {
      ret_labels.Set({i}, static_cast<typename T::Type>(labels_[i]));
      for (std::uint64_t j(0); j < 28 * 28; ++j)
      {
        ret_images.Set({i, j}, static_cast<typename T::Type>(data_[i][j]) / typename T::Type(256));
      }
    }

    return std::make_pair(ret_images, ret_labels);
  }

  std::pair<T, T> ToArray()
  {
    return SubsetToArray(size_);
  }

private:
  using uchar = unsigned char;

  uchar **read_mnist_images(std::string full_path, std::uint32_t &number_of_images,
                            unsigned int &image_size)
  {
    auto reverseInt = [](std::uint32_t i) -> std::uint32_t {
      unsigned char c1, c2, c3, c4;
      c1 = (unsigned char)(i & 255);
      c2 = (unsigned char)((i >> 8) & 255);
      c3 = (unsigned char)((i >> 16) & 255);
      c4 = (unsigned char)((i >> 24) & 255);
      return (std::uint32_t)(((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4);
    };

    std::ifstream file(full_path, std::ios::binary);

    if (file.is_open())
    {
      unsigned int magic_number = 0, n_rows = 0, n_cols = 0;

      file.read((char *)&magic_number, sizeof(magic_number));
      magic_number = reverseInt(magic_number);

      if (magic_number != 2051)
      {
        throw std::runtime_error("Invalid MNIST image file!");
      }

      file.read((char *)&number_of_images, sizeof(number_of_images)),
          number_of_images = reverseInt(number_of_images);
      file.read((char *)&n_rows, sizeof(n_rows)), n_rows = reverseInt(n_rows);
      file.read((char *)&n_cols, sizeof(n_cols)), n_cols = reverseInt(n_cols);

      image_size = n_rows * n_cols;

      uchar **_dataset = new uchar *[number_of_images];
      for (unsigned int i = 0; i < number_of_images; i++)
      {
        _dataset[i] = new uchar[image_size];
        file.read((char *)_dataset[i], std::streamsize(image_size));
      }
      return _dataset;
    }
    else
    {
      throw std::runtime_error("Cannot open file `" + full_path + "`!");
    }
  }

  uchar *read_mnist_labels(std::string full_path, std::uint32_t &number_of_labels)
  {
    auto reverseInt = [](std::uint32_t i) {
      unsigned char c1, c2, c3, c4;
      c1 = (unsigned char)(i & 255);
      c2 = (unsigned char)((i >> 8) & 255);
      c3 = (unsigned char)((i >> 16) & 255);
      c4 = (unsigned char)((i >> 24) & 255);
      return (std::uint32_t)(((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4);
    };

    std::ifstream file(full_path, std::ios::binary);

    if (file.is_open())
    {
      std::uint32_t magic_number = 0;
      file.read((char *)&magic_number, sizeof(magic_number));
      magic_number = reverseInt(magic_number);

      if (magic_number != 2049)
      {
        throw std::runtime_error("Invalid MNIST label file!");
      }

      file.read((char *)&number_of_labels, sizeof(number_of_labels)),
          number_of_labels = reverseInt(number_of_labels);

      uchar *_dataset = new uchar[number_of_labels];
      for (unsigned int i = 0; i < number_of_labels; i++)
      {
        file.read((char *)&_dataset[i], 1);
      }
      return _dataset;
    }
    else
    {
      throw std::runtime_error("Unable to open file `" + full_path + "`!");
    }
  }

private:
  std::uint32_t cursor_;
  std::uint32_t size_;

  unsigned char **data_;
  unsigned char * labels_;
};
}  // namespace ml
}  // namespace fetch
