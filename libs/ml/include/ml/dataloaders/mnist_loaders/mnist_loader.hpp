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

private:
  std::shared_ptr<SizeType> train_cursor_      = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> test_cursor_       = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> validation_cursor_ = std::make_shared<SizeType>(0);

  std::uint32_t train_size_;
  std::uint32_t test_size_;
  std::uint32_t validation_size_;

  std::uint32_t total_size_;
  std::uint32_t test_offset_;
  std::uint32_t validation_offset_;

  static constexpr std::uint32_t FIGURE_WIDTH  = 28;
  static constexpr std::uint32_t FIGURE_HEIGHT = 28;
  static constexpr std::uint32_t FIGURE_SIZE   = 28 * 28;
  static constexpr std::uint32_t LABEL_SIZE    = 10;

public:
  MNISTLoader(std::string const &images_file, std::string const &labelsFile,
              bool random_mode = false, float test_to_train_ratio = 0.0,
              float validation_to_train_ratio = 0.0)
    : DataLoader<LabelType, T>(random_mode)
  {
    std::uint32_t record_length(0);
    data_   = read_mnist_images(images_file, total_size_, record_length);
    labels_ = read_mnist_labels(labelsFile, total_size_);

    float test_percentage       = 1.0f - test_to_train_ratio - validation_to_train_ratio;
    float validation_percentage = test_percentage + test_to_train_ratio;
    assert(record_length == FIGURE_SIZE);

    test_offset_ = static_cast<std::uint32_t>(test_percentage * static_cast<float>(total_size_));
    validation_offset_ =
        static_cast<std::uint32_t>(validation_percentage * static_cast<float>(total_size_));

    validation_size_ = total_size_ - validation_offset_;
    test_size_       = validation_offset_ - test_offset_;
    train_size_      = total_size_ - test_offset_;

    // Prepare return buffer
    buffer_.second.push_back(T({FIGURE_WIDTH, FIGURE_HEIGHT, 1u}));
    buffer_.first = LabelType({LABEL_SIZE, 1u});

    *train_cursor_      = 0;
    *validation_cursor_ = validation_offset_;
    *test_cursor_       = test_offset_;

    UpdateCursor();
  }

  virtual SizeType Size() const override
  {
    // MNIST files store the size as uint32_t but Dataloader interface require uint64_t
    return this->current_size_;
  }

  virtual bool IsDone() const override
  {
    return *(this->current_cursor_) >= this->current_max_;
  }

  virtual void Reset() override
  {
    *(this->current_cursor_) = this->current_min_;
  }

  virtual ReturnType GetNext() override
  {
    if (this->random_mode_)
    {
      GetAtIndex(this->current_min_ + (static_cast<SizeType>(rand()) % this->current_size_),
                 buffer_);
      return buffer_;
    }
    else
    {
      GetAtIndex((*(this->current_cursor_))++, buffer_);
      return buffer_;
    }
  }

  inline bool IsValidable() const override
  {
    return test_size_ > 0;
  }

  void Display(T const &data) const
  {
    for (SizeType i{0}; i < FIGURE_WIDTH; ++i)
    {
      for (SizeType j{0}; j < FIGURE_HEIGHT; ++j)
      {

        std::cout << (data.At(j, i, 0) > typename T::Type(0.5) ? char(219) : ' ');
      }
      std::cout << "\n";
    }
    std::cout << std::endl;
  }

  ReturnType PrepareBatch(SizeType subset_size, bool &is_done_set) override
  {
    T ret_labels({LABEL_SIZE, subset_size});

    std::vector<T> ret_images;
    ret_images.push_back(T({FIGURE_WIDTH, FIGURE_HEIGHT, subset_size}));

    for (fetch::math::SizeType index{0}; index < subset_size; ++index)
    {

      SizeType i{0};
      auto     it = ret_images.at(0).View(index).begin();
      while (it.is_valid())
      {
        *it = static_cast<DataType>(data_[*this->current_cursor_][i]) / DataType{256};
        i++;
        ++it;
      }

      ret_labels(labels_[*this->current_cursor_], index) = static_cast<typename LabelType::Type>(1);

      if (this->random_mode_)
      {
        *this->current_cursor_ =
            this->current_min_ + (static_cast<SizeType>(rand()) % this->current_size_);
      }
      else
      {
        (*this->current_cursor_)++;
      }

      if (IsDone())
      {
        is_done_set = true;
        Reset();
      }
    }

    return std::make_pair(ret_labels, ret_images);
  }

private:
  using uchar = unsigned char;

  void UpdateCursor() override
  {
    if (this->mode_ == DataLoaderMode::TRAIN)
    {
      this->current_cursor_ = train_cursor_;
      this->current_min_    = 0;
      this->current_max_    = test_offset_;
      this->current_size_   = train_size_;
    }
    else if (this->mode_ == DataLoaderMode::TEST)
    {
      this->current_cursor_ = test_cursor_;
      this->current_min_    = test_offset_;
      this->current_max_    = validation_offset_;
      this->current_size_   = test_size_;
    }
    else if (this->mode_ == DataLoaderMode::VALIDATE)
    {
      this->current_cursor_ = validation_cursor_;
      this->current_min_    = validation_offset_;
      this->current_max_    = total_size_;
      this->current_size_   = validation_size_;
    }
    else
    {
      throw std::runtime_error("Unsupported dataloader mode.");
    }

    if (this->current_min_ == this->current_max_)
    {
      throw std::runtime_error("Dataloader has no set for selected mode.");
    }
  }

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
    buffer_.first(labels_[index], 0) = static_cast<DataType>(1.0);

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
  ReturnType buffer_;

  unsigned char **data_;
  unsigned char * labels_;
};
}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
