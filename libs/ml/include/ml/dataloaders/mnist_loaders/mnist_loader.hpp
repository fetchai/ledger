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

#include "core/macros.hpp"
#include "core/random.hpp"
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

template <typename LabelType, typename InputType>
class MNISTLoader : public DataLoader<LabelType, InputType>
{
public:
  using SizeType   = typename InputType::SizeType;
  using DataType   = typename InputType::Type;
  using ReturnType = std::pair<LabelType, std::vector<InputType>>;

private:
  std::shared_ptr<SizeType> train_cursor_      = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> test_cursor_       = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> validation_cursor_ = std::make_shared<SizeType>(0);

  uint32_t train_size_{};
  uint32_t test_size_{};
  uint32_t validation_size_{};

  uint32_t total_size_{};
  uint32_t test_offset_{};
  uint32_t validation_offset_{};

  float test_to_train_ratio_       = 0.0f;
  float validation_to_train_ratio_ = 0.0f;

  static constexpr uint32_t FIGURE_WIDTH  = 28;
  static constexpr uint32_t FIGURE_HEIGHT = 28;
  static constexpr uint32_t FIGURE_SIZE   = 28 * 28;
  static constexpr uint32_t LABEL_SIZE    = 10;

public:
  explicit MNISTLoader(bool random_mode = false)
    : DataLoader<LabelType, InputType>()
  {
    // Prepare return buffer
    this->SetRandomMode(random_mode);
    buffer_.second.push_back(InputType({FIGURE_WIDTH, FIGURE_HEIGHT, 1u}));
    buffer_.first = LabelType({LABEL_SIZE, 1u});
    UpdateRanges();
  }

  MNISTLoader(std::string const &images_file, std::string const &labels_file)
    : DataLoader<LabelType, InputType>()
  {
    SetupWithDataFiles(images_file, labels_file);
  }

  SizeType Size() const override
  {
    // MNIST files store the size as uint32_t but Dataloader interface require uint64_t
    return this->current_size_;
  }

  bool IsDone() const override
  {
    return *(this->current_cursor_) >= this->current_max_;
  }

  /**
   * Resets current cursor to beginning
   */
  void Reset() override
  {
    *(this->current_cursor_) = this->current_min_;
  }

  ReturnType GetNext() override
  {
    if (this->random_mode_)
    {
      GetAtIndex(this->current_min_ + (static_cast<SizeType>(fetch::random::Random::generator()) %
                                       this->current_size_),
                 buffer_);
      return buffer_;
    }

    GetAtIndex((*(this->current_cursor_))++, buffer_);
    return buffer_;
  }

  void SetTestRatio(float new_test_ratio) override
  {
    test_to_train_ratio_ = new_test_ratio;
    UpdateRanges();
  }

  void SetValidationRatio(float new_validation_ratio) override
  {
    validation_to_train_ratio_ = new_validation_ratio;
    UpdateRanges();
  }

  /**
   * directly sets the data and labels variables
   * This function must be implemented to override from Dataloader, but we
   * may prefer to use SetupWithDataFiles helper function
   * @param data
   * @param label
   * @return
   */
  bool AddData(InputType const &data, LabelType const &label) override
  {
    FETCH_UNUSED(data);
    FETCH_UNUSED(label);
    throw exceptions::InvalidMode(
        "AddData not implemented for MNist example - please use Constructor or SetupWithDataFiles "
        "methods");
  }

  ReturnType PrepareBatch(SizeType subset_size, bool &is_done_set) override
  {
    InputType ret_labels({LABEL_SIZE, subset_size});

    std::vector<InputType> ret_images;
    ret_images.push_back(InputType({FIGURE_WIDTH, FIGURE_HEIGHT, subset_size}));

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
            this->current_min_ +
            (static_cast<SizeType>(fetch::random::Random::generator()) % this->current_size_);
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

  void SetupWithDataFiles(std::string const &images_file, std::string const &labels_file)
  {
    uint32_t record_length(0);
    data_   = ReadMnistImages(images_file, total_size_, record_length);
    labels_ = ReadMNistLabels(labels_file, total_size_);
    assert(record_length == FIGURE_SIZE);

    // Prepare return buffer
    buffer_.first = LabelType({LABEL_SIZE, 1u});
    buffer_.second.clear();
    buffer_.second.push_back(InputType({FIGURE_WIDTH, FIGURE_HEIGHT, 1u}));
    UpdateRanges();
  }

  static uint8_t **ReadMnistImages(std::string const &full_path, uint32_t &number_of_images,
                                   unsigned int &image_size)
  {
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

    std::ifstream file(full_path, std::ios::binary);

    if (file.is_open())
    {
      unsigned int magic_number = 0, n_rows = 0, n_cols = 0;

      file.read(reinterpret_cast<char *>(&magic_number), sizeof(magic_number));
      magic_number = reverseInt(magic_number);

      if (magic_number != 2051)
      {
        throw exceptions::InvalidFile("Invalid MNIST image file!");
      }

      file.read(reinterpret_cast<char *>(&number_of_images), sizeof(number_of_images)),
          number_of_images = reverseInt(number_of_images);
      file.read(reinterpret_cast<char *>(&n_rows), sizeof(n_rows)), n_rows = reverseInt(n_rows);
      file.read(reinterpret_cast<char *>(&n_cols), sizeof(n_cols)), n_cols = reverseInt(n_cols);

      image_size = n_rows * n_cols;

      auto **_dataset = new UnsignedChar *[number_of_images];
      for (unsigned int i = 0; i < number_of_images; i++)
      {
        _dataset[i] = new UnsignedChar[image_size];
        file.read(reinterpret_cast<char *>(_dataset[i]), std::streamsize(image_size));
      }
      return _dataset;
    }

    throw exceptions::InvalidFile("Cannot open file `" + full_path + "`!");
  }

  static uint8_t *ReadMNistLabels(std::string const &full_path, uint32_t &number_of_labels)
  {
    auto reverseInt = [](uint32_t i) {
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

    std::ifstream file(full_path, std::ios::binary);

    if (file.is_open())
    {
      uint32_t magic_number = 0;
      file.read(reinterpret_cast<char *>(&magic_number), sizeof(magic_number));
      magic_number = reverseInt(magic_number);

      if (magic_number != 2049)
      {
        throw exceptions::InvalidFile("Invalid MNIST label file!");
      }

      file.read(reinterpret_cast<char *>(&number_of_labels), sizeof(number_of_labels)),
          number_of_labels = reverseInt(number_of_labels);

      auto *_dataset = new UnsignedChar[number_of_labels];
      for (unsigned int i = 0; i < number_of_labels; i++)
      {
        file.read(reinterpret_cast<char *>(&_dataset[i]), 1);
      }
      return _dataset;
    }

    throw exceptions::InvalidFile("Unable to open file `" + full_path + "`!");
  }

private:
  using UnsignedChar = uint8_t;

  void UpdateCursor() override
  {
    switch (this->mode_)
    {
    case DataLoaderMode::TRAIN:
    {
      this->current_cursor_ = train_cursor_;
      this->current_min_    = 0;
      this->current_max_    = test_offset_;
      this->current_size_   = train_size_;
      break;
    }
    case DataLoaderMode::TEST:
    {
      this->current_cursor_ = test_cursor_;
      this->current_min_    = test_offset_;
      this->current_max_    = validation_offset_;
      this->current_size_   = test_size_;
      break;
    }
    case DataLoaderMode::VALIDATE:
    {
      this->current_cursor_ = validation_cursor_;
      this->current_min_    = validation_offset_;
      this->current_max_    = total_size_;
      this->current_size_   = validation_size_;
      break;
    }
    default:
    {
      throw exceptions::InvalidMode("Unsupported dataloader mode.");
    }
    }
  }

  bool IsModeAvailable(DataLoaderMode mode) override
  {
    switch (mode)
    {
    case DataLoaderMode::TRAIN:
    {
      return test_offset_ > 0;
    }
    case DataLoaderMode::TEST:
    {
      return test_offset_ < validation_offset_;
    }
    case DataLoaderMode::VALIDATE:
    {
      return validation_offset_ < total_size_;
    }
    default:
    {
      throw exceptions::InvalidMode("Unsupported dataloader mode.");
    }
    }
  }

  void UpdateRanges()
  {
    float test_percentage       = 1.0f - test_to_train_ratio_ - validation_to_train_ratio_;
    float validation_percentage = test_percentage + test_to_train_ratio_;

    // Define where test set starts
    test_offset_ = static_cast<uint32_t>(test_percentage * static_cast<float>(total_size_));

    if (test_offset_ == static_cast<SizeType>(0))
    {
      test_offset_ = static_cast<SizeType>(1);
    }

    // Define where validation set starts
    validation_offset_ =
        static_cast<uint32_t>(validation_percentage * static_cast<float>(total_size_));

    if (validation_offset_ <= test_offset_)
    {
      validation_offset_ = test_offset_ + 1;
    }

    // boundary check and fix
    if (validation_offset_ > total_size_)
    {
      validation_offset_ = total_size_;
    }

    if (test_offset_ > total_size_)
    {
      test_offset_ = total_size_;
    }

    validation_size_ = total_size_ - validation_offset_;
    test_size_       = validation_offset_ - test_offset_;
    train_size_      = test_offset_;

    *train_cursor_      = 0;
    *test_cursor_       = test_offset_;
    *validation_cursor_ = validation_offset_;

    UpdateCursor();
  }

  void GetAtIndex(SizeType index, ReturnType &ret)
  {
    SizeType i{0};

    for (auto &val : buffer_.second.at(0))
    {
      val = static_cast<DataType>(data_[index][i]) / DataType{256};
      i++;
    }

    buffer_.first.Fill(DataType{0});
    buffer_.first(labels_[index], 0) = static_cast<DataType>(1.0);

    ret = buffer_;
  }

private:
  ReturnType buffer_;

  uint8_t **data_{};
  uint8_t * labels_{};
};
}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
