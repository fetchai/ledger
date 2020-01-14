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

#include "core/serializers/group_definitions.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/meta/ml_type_traits.hpp"

#include <stdexcept>
#include <utility>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename TensorType>
class TensorDataLoader : public DataLoader<TensorType>
{
  using DataType = typename TensorType::Type;

  using SizeType     = fetch::math::SizeType;
  using SizeVector   = fetch::math::SizeVector;
  using ReturnType   = std::pair<TensorType, std::vector<TensorType>>;
  using IteratorType = typename TensorType::IteratorType;

public:
  TensorDataLoader() = default;

  ~TensorDataLoader() override = default;

  ReturnType GetNext() override;

  bool AddData(std::vector<TensorType> const &data, TensorType const &labels) override;

  SizeType Size() const override;
  bool     IsDone() const override;
  void     Reset() override;
  bool     IsModeAvailable(DataLoaderMode mode) override;

  void SetTestRatio(DataType new_test_ratio) override;
  void SetValidationRatio(DataType new_validation_ratio) override;

  template <typename X, typename D>
  friend struct fetch::serializers::MapSerializer;

  LoaderType LoaderCode() override
  {
    return LoaderType::TENSOR;
  }

protected:
  std::shared_ptr<SizeType> train_cursor_      = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> test_cursor_       = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> validation_cursor_ = std::make_shared<SizeType>(0);

  SizeType test_offset_       = 0;
  SizeType validation_offset_ = 0;

  SizeType n_samples_            = 0;  // number of all samples
  SizeType n_test_samples_       = 0;  // number of test samples
  SizeType n_validation_samples_ = 0;  // number of validation samples
  SizeType n_train_samples_      = 0;  // number of train samples

  std::vector<TensorType> data_;
  TensorType              labels_;

  SizeVector              one_sample_label_shape_;
  std::vector<SizeVector> one_sample_data_shapes_;
  DataType                test_to_train_ratio_       = DataType{0};
  DataType                validation_to_train_ratio_ = DataType{0};

  SizeType batch_label_dim_ = fetch::math::numeric_max<SizeType>();
  SizeType batch_data_dim_  = fetch::math::numeric_max<SizeType>();

  std::shared_ptr<SizeType> train_count_      = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> test_count_       = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> validation_count_ = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> count_            = train_count_;

  void UpdateRanges();
  void UpdateCursor() override;
};

template <typename TensorType>
typename TensorDataLoader<TensorType>::ReturnType TensorDataLoader<TensorType>::GetNext()
{
  std::vector<TensorType> ret_data;
  TensorType ret_labels = labels_.View(*this->current_cursor_).Copy(one_sample_label_shape_);

  for (SizeType i{0}; i < data_.size(); i++)
  {
    ret_data.emplace_back(
        data_.at(i).View(*this->current_cursor_).Copy(one_sample_data_shapes_.at(i)));
  }

  if (this->random_mode_)
  {
    *this->current_cursor_ = this->current_min_ + SizeType{this->rand()} % this->current_size_;
    ++(*count_);
  }
  else
  {
    (*this->current_cursor_)++;
  }

  return ReturnType(ret_labels, ret_data);
}

template <typename TensorType>
bool TensorDataLoader<TensorType>::AddData(std::vector<TensorType> const &data,
                                           TensorType const &             labels)
{
  one_sample_label_shape_                                        = labels.shape();
  one_sample_label_shape_.at(one_sample_label_shape_.size() - 1) = 1;
  labels_                                                        = labels.Copy();

  // Resize data vector
  if (data_.size() < data.size())
  {
    data_.resize(data.size());
    one_sample_data_shapes_.resize(data.size());
  }

  // Add data to data vector
  for (SizeType i{0}; i < data.size(); i++)
  {
    data_.at(i)                                                                = data.at(i).Copy();
    one_sample_data_shapes_.at(i)                                              = data.at(i).shape();
    one_sample_data_shapes_.at(i).at(one_sample_data_shapes_.at(i).size() - 1) = 1;
  }

  n_samples_ = data_.at(0).shape().at(data_.at(0).shape().size() - 1);

  UpdateRanges();

  return true;
}

template <typename TensorType>
typename TensorDataLoader<TensorType>::SizeType TensorDataLoader<TensorType>::Size() const
{
  return this->current_size_;
}

template <typename TensorType>
bool TensorDataLoader<TensorType>::IsDone() const
{
  if (this->random_mode_)
  {
    return (*count_ > (this->current_max_ - this->current_min_));
  }

  return *(this->current_cursor_) >= this->current_max_;
}

template <typename TensorType>
void TensorDataLoader<TensorType>::Reset()
{
  *count_                  = 0;
  *(this->current_cursor_) = this->current_min_;
}

template <typename TensorType>
void TensorDataLoader<TensorType>::SetTestRatio(DataType new_test_ratio)
{
  test_to_train_ratio_ = new_test_ratio;
  UpdateRanges();
}

template <typename TensorType>
void TensorDataLoader<TensorType>::SetValidationRatio(DataType new_validation_ratio)
{
  validation_to_train_ratio_ = new_validation_ratio;
  UpdateRanges();
}

template <typename TensorType>
void TensorDataLoader<TensorType>::UpdateRanges()
{
  DataType test_percentage       = DataType{1} - test_to_train_ratio_ - validation_to_train_ratio_;
  DataType validation_percentage = test_percentage + test_to_train_ratio_;

  // Define where test set starts
  test_offset_ = static_cast<uint32_t>(test_percentage * static_cast<DataType>(n_samples_));

  if (test_offset_ == static_cast<SizeType>(0))
  {
    test_offset_ = static_cast<SizeType>(1);
  }

  // Define where validation set starts
  validation_offset_ =
      static_cast<uint32_t>(validation_percentage * static_cast<DataType>(n_samples_));

  if (validation_offset_ <= test_offset_)
  {
    validation_offset_ = test_offset_ + 1;
  }

  // boundary check and fix
  if (validation_offset_ > n_samples_)
  {
    validation_offset_ = n_samples_;
  }

  if (test_offset_ > n_samples_)
  {
    test_offset_ = n_samples_;
  }

  n_validation_samples_ = n_samples_ - validation_offset_;
  n_test_samples_       = validation_offset_ - test_offset_;
  n_train_samples_      = test_offset_;

  *train_cursor_      = 0;
  *test_cursor_       = test_offset_;
  *validation_cursor_ = validation_offset_;

  UpdateCursor();
}

template <typename TensorType>
void TensorDataLoader<TensorType>::UpdateCursor()
{
  switch (this->mode_)
  {
  case DataLoaderMode::TRAIN:
  {
    this->current_cursor_ = train_cursor_;
    this->current_min_    = 0;
    this->current_max_    = test_offset_;
    this->current_size_   = n_train_samples_;
    count_                = train_count_;
    break;
  }
  case DataLoaderMode::TEST:
  {
    if (test_to_train_ratio_ == 0)
    {
      throw exceptions::InvalidMode("Dataloader has no test set.");
    }
    this->current_cursor_ = test_cursor_;
    this->current_min_    = test_offset_;
    this->current_max_    = validation_offset_;
    this->current_size_   = n_test_samples_;
    count_                = test_count_;
    break;
  }
  case DataLoaderMode::VALIDATE:
  {
    if (validation_to_train_ratio_ == 0)
    {
      throw exceptions::InvalidMode("Dataloader has no validation set.");
    }
    this->current_cursor_ = validation_cursor_;
    this->current_min_    = validation_offset_;
    this->current_max_    = n_samples_;
    this->current_size_   = n_validation_samples_;
    count_                = validation_count_;
    break;
  }
  default:
  {
    throw exceptions::InvalidMode("Unsupported dataloader mode.");
  }
  }
}

template <typename TensorType>
bool TensorDataLoader<TensorType>::IsModeAvailable(DataLoaderMode mode)
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
    return validation_offset_ < n_samples_;
  }
  default:
  {
    throw exceptions::InvalidMode("Unsupported dataloader mode.");
  }
  }
}

}  // namespace dataloaders
}  // namespace ml

namespace serializers {

/**
 * serializer for tensor dataloader
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<fetch::ml::dataloaders::TensorDataLoader<TensorType>, D>
{
  using Type       = fetch::ml::dataloaders::TensorDataLoader<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_DATA_LOADER          = 1;
  static uint8_t const TRAIN_CURSOR              = 2;
  static uint8_t const TEST_CURSOR               = 3;
  static uint8_t const VALIDATION_CURSOR         = 4;
  static uint8_t const TEST_OFFSET               = 5;
  static uint8_t const VALIDATION_OFFSET         = 6;
  static uint8_t const TEST_TO_TRAIN_RATIO       = 7;
  static uint8_t const VALIDATION_TO_TRAIN_RATIO = 8;
  static uint8_t const N_SAMPLES                 = 9;
  static uint8_t const N_TRAIN_SAMPLES           = 10;
  static uint8_t const N_TEST_SAMPLES            = 11;
  static uint8_t const N_VALIDATION_SAMPLES      = 12;
  static uint8_t const DATA                      = 13;
  static uint8_t const LABELS                    = 14;

  static uint8_t const ONE_SAMPLE_LABEL_SHAPE = 15;
  static uint8_t const ONE_SAMPLE_DATA_SHAPES = 16;

  static uint8_t const BATCH_LABEL_DIM  = 17;
  static uint8_t const BATCH_DATA_DIM   = 18;
  static uint8_t const TRAIN_COUNT      = 19;
  static uint8_t const TEST_COUNT       = 20;
  static uint8_t const VALIDATION_COUNT = 21;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(21);

    // serialize parent class first
    auto dl_pointer = static_cast<ml::dataloaders::DataLoader<TensorType> const *>(&sp);
    map.Append(BASE_DATA_LOADER, *(dl_pointer));

    map.Append(TRAIN_CURSOR, *sp.train_cursor_);
    map.Append(TEST_CURSOR, *sp.test_cursor_);
    map.Append(VALIDATION_CURSOR, *sp.validation_cursor_);

    map.Append(TEST_OFFSET, sp.test_offset_);
    map.Append(VALIDATION_OFFSET, sp.validation_offset_);

    map.Append(TEST_TO_TRAIN_RATIO, sp.test_to_train_ratio_);
    map.Append(VALIDATION_TO_TRAIN_RATIO, sp.validation_to_train_ratio_);

    map.Append(N_SAMPLES, sp.n_samples_);
    map.Append(N_TRAIN_SAMPLES, sp.n_train_samples_);
    map.Append(N_TEST_SAMPLES, sp.n_test_samples_);
    map.Append(N_VALIDATION_SAMPLES, sp.n_validation_samples_);

    map.Append(DATA, sp.data_);
    map.Append(LABELS, sp.labels_);

    map.Append(ONE_SAMPLE_LABEL_SHAPE, sp.one_sample_label_shape_);
    map.Append(ONE_SAMPLE_DATA_SHAPES, sp.one_sample_data_shapes_);

    map.Append(BATCH_LABEL_DIM, sp.batch_label_dim_);
    map.Append(BATCH_DATA_DIM, sp.batch_data_dim_);
    map.Append(TRAIN_COUNT, *sp.train_count_);
    map.Append(TEST_COUNT, *sp.test_count_);
    map.Append(VALIDATION_COUNT, *sp.validation_count_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto dl_pointer = static_cast<ml::dataloaders::DataLoader<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_DATA_LOADER, (*dl_pointer));

    map.ExpectKeyGetValue(TRAIN_CURSOR, *sp.train_cursor_);
    map.ExpectKeyGetValue(TEST_CURSOR, *sp.test_cursor_);
    map.ExpectKeyGetValue(VALIDATION_CURSOR, *sp.validation_cursor_);

    map.ExpectKeyGetValue(TEST_OFFSET, sp.test_offset_);
    map.ExpectKeyGetValue(VALIDATION_OFFSET, sp.validation_offset_);

    map.ExpectKeyGetValue(TEST_TO_TRAIN_RATIO, sp.test_to_train_ratio_);
    map.ExpectKeyGetValue(VALIDATION_TO_TRAIN_RATIO, sp.validation_to_train_ratio_);

    map.ExpectKeyGetValue(N_SAMPLES, sp.n_samples_);
    map.ExpectKeyGetValue(N_TRAIN_SAMPLES, sp.n_train_samples_);
    map.ExpectKeyGetValue(N_TEST_SAMPLES, sp.n_test_samples_);
    map.ExpectKeyGetValue(N_VALIDATION_SAMPLES, sp.n_validation_samples_);

    map.ExpectKeyGetValue(DATA, sp.data_);
    map.ExpectKeyGetValue(LABELS, sp.labels_);

    map.ExpectKeyGetValue(ONE_SAMPLE_LABEL_SHAPE, sp.one_sample_label_shape_);
    map.ExpectKeyGetValue(ONE_SAMPLE_DATA_SHAPES, sp.one_sample_data_shapes_);

    map.ExpectKeyGetValue(BATCH_LABEL_DIM, sp.batch_label_dim_);
    map.ExpectKeyGetValue(BATCH_DATA_DIM, sp.batch_data_dim_);
    map.ExpectKeyGetValue(TRAIN_COUNT, *sp.train_count_);
    map.ExpectKeyGetValue(TEST_COUNT, *sp.test_count_);
    map.ExpectKeyGetValue(VALIDATION_COUNT, *sp.validation_count_);
    sp.UpdateCursor();
  }
};

}  // namespace serializers

}  // namespace fetch
