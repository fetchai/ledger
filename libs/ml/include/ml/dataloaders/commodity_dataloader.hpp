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

#include "core/random.hpp"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"

#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename LabelType, typename InputType>
class CommodityDataLoader : public DataLoader<LabelType, InputType>
{
public:
  using DataType   = typename InputType::Type;
  using ReturnType = std::pair<LabelType, std::vector<InputType>>;
  using SizeType   = math::SizeType;

  CommodityDataLoader()
    : DataLoader<LabelType, InputType>()
  {
    UpdateRanges();
  }

  ~CommodityDataLoader() override = default;

  ReturnType GetNext() override;
  SizeType   Size() const override;
  bool       IsDone() const override;
  void       Reset() override;
  bool       IsModeAvailable(DataLoaderMode mode) override;

  void SetTestRatio(float new_test_ratio) override;
  void SetValidationRatio(float new_validation_ratio) override;

  bool AddData(InputType const &data, LabelType const &label) override;

private:
  bool      random_mode_ = false;
  InputType data_;    // n_data, features
  LabelType labels_;  // n_data, features

  SizeType   rows_to_skip_ = 1;
  SizeType   cols_to_skip_ = 1;
  ReturnType buffer_;

  SizeType size_ = 0;

  std::shared_ptr<SizeType> train_cursor_      = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> test_cursor_       = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> validation_cursor_ = std::make_shared<SizeType>(0);

  SizeType train_size_{};
  SizeType test_size_{};
  SizeType validation_size_{};

  SizeType total_size_{};
  SizeType test_offset_{};
  SizeType validation_offset_{};

  float test_to_train_ratio_       = 0.0f;
  float validation_to_train_ratio_ = 0.0f;

  random::Random rand_;

  void GetAtIndex(SizeType index);
  void UpdateCursor() override;
  void UpdateRanges();
};

/**
 * Adds input data and labels of commodity data.
 * By default this skips the first row and column as these are assumed to be indices
 * @tparam LabelType
 * @tparam InputType
 * @param data
 * @param label
 * @return
 */
template <typename LabelType, typename InputType>
bool CommodityDataLoader<LabelType, InputType>::AddData(InputType const &data,
                                                        LabelType const &label)
{
  data_   = data;
  labels_ = label;
  assert(data_.shape().at(1) == labels_.shape().at(1));
  size_ = data.shape().at(1);

  UpdateRanges();

  return true;
}

/**
 * Gets the next pair of data and labels
 * @tparam LabelType type for the labels
 * @tparam InputType type for the data
 * @return Pair of input and output
 */
template <typename LabelType, typename InputType>
typename CommodityDataLoader<LabelType, InputType>::ReturnType
CommodityDataLoader<LabelType, InputType>::GetNext()
{
  if (this->random_mode_)
  {
    GetAtIndex(this->current_min_ + (static_cast<SizeType>(decltype(rand_)::generator()) % Size()));
    return buffer_;
  }

  GetAtIndex((*this->current_cursor_)++);
  return buffer_;
}

/**
 * Returns the number of datapoints
 * @return number of datapoints
 */
template <typename LabelType, typename InputType>
typename CommodityDataLoader<LabelType, InputType>::SizeType
CommodityDataLoader<LabelType, InputType>::Size() const
{
  return static_cast<SizeType>(this->current_size_);
}

template <typename LabelType, typename InputType>
bool CommodityDataLoader<LabelType, InputType>::IsDone() const
{
  return *this->current_cursor_ >= this->current_max_;
}

/**
 * Resets current cursor to beginning
 */
template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::Reset()
{
  *(this->current_cursor_) = this->current_min_;
}

template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::SetTestRatio(float new_test_ratio)
{
  FETCH_UNUSED(new_test_ratio);
  throw exceptions::InvalidMode("Test set splitting is not supported for this dataloader.");
}

template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::SetValidationRatio(float new_validation_ratio)
{
  FETCH_UNUSED(new_validation_ratio);
  throw exceptions::InvalidMode("Validation set splitting is not supported for this dataloader.");
}

/**
 * Returns the data pair at the given index
 * @param index index of the data
 */
template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::GetAtIndex(CommodityDataLoader::SizeType index)
{
  buffer_.first  = labels_.View(index).Copy();
  buffer_.second = std::vector<InputType>({data_.View(index).Copy()});
}

template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::UpdateCursor()
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
    throw exceptions::InvalidMode("Unsupported dataloader mode.");
  }
}

template <typename LabelType, typename InputType>
bool CommodityDataLoader<LabelType, InputType>::IsModeAvailable(DataLoaderMode mode)
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

template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::UpdateRanges()
{
  float test_percentage       = 1.0f - test_to_train_ratio_ - validation_to_train_ratio_;
  float validation_percentage = test_percentage + test_to_train_ratio_;

  // Define where test set starts
  test_offset_ = static_cast<uint32_t>(test_percentage * static_cast<float>(size_));

  if (test_offset_ == static_cast<SizeType>(0))
  {
    test_offset_ = static_cast<SizeType>(1);
  }

  // Define where validation set starts
  validation_offset_ = static_cast<uint32_t>(validation_percentage * static_cast<float>(size_));

  if (validation_offset_ <= test_offset_)
  {
    validation_offset_ = test_offset_ + 1;
  }

  // boundary check and fix
  if (validation_offset_ > size_)
  {
    validation_offset_ = size_;
  }

  if (test_offset_ > size_)
  {
    test_offset_ = size_;
  }

  validation_size_ = size_ - validation_offset_;
  test_size_       = validation_offset_ - test_offset_;
  train_size_      = test_offset_;

  *train_cursor_      = 0;
  *test_cursor_       = test_offset_;
  *validation_cursor_ = validation_offset_;

  UpdateCursor();
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
