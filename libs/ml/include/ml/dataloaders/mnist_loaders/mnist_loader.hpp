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

#include "math/base_types.hpp"
#include "math/meta/math_type_traits.hpp"
#include "ml/dataloaders/dataloader.hpp"

#include <exception>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename LabelType, typename T>
class MNISTLoader : public DataLoader<LabelType, T>
{
public:
  using SizeType   = typename T::SizeType;
  using DataType   = typename T::Type;
  using ReturnType = std::pair<LabelType, std::vector<T>>;

  MNISTLoader(std::string const &imagesFile, std::string const &labelsFile,
              bool random_mode = false)
    : DataLoader<LabelType, T>(random_mode)
    , cursor_(0)
  {
    std::uint32_t recordLength(0);
    data_        = read_mnist_images(imagesFile, size_, recordLength);
    labels_      = read_mnist_labels(labelsFile, size_);
    figure_size_ = 28 * 28;
    assert(recordLength == figure_size_);

    // Prepare return buffer
    buffer_.second.push_back(T({28u, 28u, 1u}));
    buffer_.first = LabelType({10u, 1u});
  }

  virtual SizeType Size() const
  {
    // MNIST files store the size as uint32_t but Dataloader interface require uint64_t
    return static_cast<SizeType>(size_);
  }

  virtual bool IsDone() const
  {
    return cursor_ >= size_;
  }

  virtual void Reset()
  {
    cursor_ = 0;
  }

  virtual ReturnType GetNext()
  {
    if (this->random_mode_)
    {
      GetAtIndex((SizeType)rand() % Size(), buffer_);
      return buffer_;
    }
    else
    {
      GetAtIndex(static_cast<SizeType>(cursor_++), buffer_);
      return buffer_;
    }
  }

  void Display(T const &data) const
  {
    for (SizeType i{0}; i < 28u; ++i)
    {
      for (SizeType j{0}; j < 28u; ++j)
      {

        std::cout << (data.At(j, i, 0) > typename T::Type(0.5) ? char(219) : ' ');
      }
      std::cout << "\n";
    }
    std::cout << std::endl;
  }

  ReturnType SubsetToArray(SizeType subset_size)
  {
    T              ret_labels({1, subset_size});
    std::vector<T> ret_images;
    ret_images.push_back(T({figure_size_, subset_size}));

    for (fetch::math::SizeType i(0); i < subset_size; ++i)
    {
      ret_labels(0, i) = static_cast<typename T::Type>(labels_[i]);
      for (fetch::math::SizeType j{0}; j < figure_size_; ++j)
      {
        ret_images.at(0)(j, i) = static_cast<typename T::Type>(data_[i][j]) / typename T::Type(256);
      }
    }

    return std::make_pair(ret_labels, ret_images);
  }

  std::pair<LabelType, std::vector<T>> ToArray()
  {
    return SubsetToArray(size_);
  }

private:
  using uchar = unsigned char;

  void GetAtIndex(SizeType index, ReturnType &ret)
  {
    SizeType i{0};
    auto     it = buffer_.second.at(0).begin();
    while (it.is_valid())
    {
      *it = static_cast<DataType>(data_[index][i]) / DataType{256};
      i++;
      ++it;
    }

    buffer_.first.Fill(DataType{0});
    buffer_.first(labels_[index], 0) = DataType{1.0};

    ret = buffer_;
  }

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
  std::uint32_t figure_size_;

  ReturnType buffer_;

  unsigned char **data_;
  unsigned char * labels_;
};
}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
