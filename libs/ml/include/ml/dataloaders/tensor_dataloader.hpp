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
#include "core/serializers/group_definitions.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename LabelType, typename InputType>
class TensorDataLoader : public DataLoader<LabelType, InputType>
{
  using TensorType = InputType;
  using DataType   = typename TensorType::Type;

  using SizeType     = fetch::math::SizeType;
  using SizeVector   = fetch::math::SizeVector;
  using ReturnType   = std::pair<LabelType, std::vector<TensorType>>;
  using IteratorType = typename TensorType::IteratorType;

public:
  TensorDataLoader() = default;
  TensorDataLoader(SizeVector label_shape, std::vector<SizeVector> data_shapes);

  ~TensorDataLoader() override = default;

  ReturnType GetNext() override;

  bool AddData(InputType const &data, LabelType const &labels) override;

  SizeType Size() const override;
  bool     IsDone() const override;
  void     Reset() override;
  bool     IsModeAvailable(DataLoaderMode mode) override;

  void SetTestRatio(float new_test_ratio) override;
  void SetValidationRatio(float new_validation_ratio) override;

  template <typename X, typename D>
  friend struct fetch::serializers::MapSerializer;

protected:
  std::shared_ptr<SizeType> train_cursor_      = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> test_cursor_       = std::make_shared<SizeType>(0);
  std::shared_ptr<SizeType> validation_cursor_ = std::make_shared<SizeType>(0);

  SizeType test_offset_       = 0;
  SizeType validation_offset_ = 0;

  SizeType n_samples_            = 0;  // number of all samples
  SizeType n_test_samples_       = 0;  // number of train samples
  SizeType n_validation_samples_ = 0;  // number of train samples
  SizeType n_train_samples_      = 0;  // number of validation samples

  TensorType data_;
  TensorType labels_;

  SizeVector              label_shape_;
  SizeVector              one_sample_label_shape_;
  std::vector<SizeVector> data_shapes_;
  std::vector<SizeVector> one_sample_data_shapes_;
  float                   test_to_train_ratio_       = 0.0;
  float                   validation_to_train_ratio_ = 0.0;

  SizeType batch_label_dim_ = fetch::math::numeric_max<SizeType>();
  SizeType batch_data_dim_  = fetch::math::numeric_max<SizeType>();

  random::Random rand;
  SizeType       count_ = 0;

  void UpdateRanges();
  void UpdateCursor() override;
};

template <typename LabelType, typename InputType>
TensorDataLoader<LabelType, InputType>::TensorDataLoader(SizeVector              label_shape,
                                                         std::vector<SizeVector> data_shapes)
  : DataLoader<LabelType, TensorType>()
  , label_shape_(std::move(label_shape))
  , data_shapes_(std::move(data_shapes))
{
  UpdateCursor();
}

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::ReturnType
TensorDataLoader<LabelType, InputType>::GetNext()
{
  ReturnType ret(labels_.View(*this->current_cursor_).Copy(one_sample_label_shape_),
                 {data_.View(*this->current_cursor_).Copy(one_sample_data_shapes_.at(0))});

  if (this->random_mode_)
  {
    *this->current_cursor_ =
        this->current_min_ +
        (static_cast<SizeType>(decltype(rand)::generator()) % this->current_size_);
    ++count_;
  }
  else
  {
    (*this->current_cursor_)++;
  }

  return ret;
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::AddData(InputType const &data, LabelType const &labels)
{
  one_sample_label_shape_                                        = labels.shape();
  one_sample_label_shape_.at(one_sample_label_shape_.size() - 1) = 1;

  one_sample_data_shapes_.emplace_back(data.shape());
  one_sample_data_shapes_.at(0).at(one_sample_data_shapes_.at(0).size() - 1) = 1;

  data_   = data.Copy();
  labels_ = labels.Copy();

  n_samples_ = data_.shape().at(data_.shape().size() - 1);

  UpdateRanges();

  return true;
}

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::SizeType
TensorDataLoader<LabelType, InputType>::Size() const
{
  return this->current_size_;
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::IsDone() const
{
  if (this->random_mode_)
  {
    return (count_ > (this->current_max_ - this->current_min_));
  }

  return *(this->current_cursor_) >= this->current_max_;
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::Reset()
{
  count_                   = 0;
  *(this->current_cursor_) = this->current_min_;
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::SetTestRatio(float new_test_ratio)
{
  test_to_train_ratio_ = new_test_ratio;
  UpdateRanges();
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::SetValidationRatio(float new_validation_ratio)
{
  validation_to_train_ratio_ = new_validation_ratio;
  UpdateRanges();
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::UpdateRanges()
{
  float test_percentage       = 1.0f - test_to_train_ratio_ - validation_to_train_ratio_;
  float validation_percentage = test_percentage + test_to_train_ratio_;

  // Define where test set starts
  test_offset_ = static_cast<uint32_t>(test_percentage * static_cast<float>(n_samples_));

  if (test_offset_ == static_cast<SizeType>(0))
  {
    test_offset_ = static_cast<SizeType>(1);
  }

  // Define where validation set starts
  validation_offset_ =
      static_cast<uint32_t>(validation_percentage * static_cast<float>(n_samples_));

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

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::UpdateCursor()
{
  switch (this->mode_)
  {
  case DataLoaderMode::TRAIN:
  {
    this->current_cursor_ = train_cursor_;
    this->current_min_    = 0;
    this->current_max_    = test_offset_;
    this->current_size_   = n_train_samples_;
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
    break;
  }
  default:
  {
    throw exceptions::InvalidMode("Unsupported dataloader mode.");
  }
  }
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::IsModeAvailable(DataLoaderMode mode)
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
template <typename LabelType, typename InputType, typename D>
struct MapSerializer<fetch::ml::dataloaders::TensorDataLoader<LabelType, InputType>, D>
{
  using Type       = fetch::ml::dataloaders::TensorDataLoader<LabelType, InputType>;
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

  static uint8_t const LABEL_SHAPE            = 15;
  static uint8_t const ONE_SAMPLE_LABEL_SHAPE = 16;
  static uint8_t const DATA_SHAPES            = 17;
  static uint8_t const ONE_SAMPLE_DATA_SHAPES = 18;

  static uint8_t const BATCH_LABEL_DIM = 19;
  static uint8_t const BATCH_DATA_DIM  = 20;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(20);

    // serialize parent class first
    auto dl_pointer = static_cast<ml::dataloaders::DataLoader<LabelType, InputType> const *>(&sp);
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

    map.Append(LABEL_SHAPE, sp.label_shape_);
    map.Append(ONE_SAMPLE_LABEL_SHAPE, sp.one_sample_label_shape_);
    map.Append(DATA_SHAPES, sp.data_shapes_);
    map.Append(ONE_SAMPLE_DATA_SHAPES, sp.one_sample_data_shapes_);

    map.Append(BATCH_LABEL_DIM, sp.batch_label_dim_);
    map.Append(BATCH_DATA_DIM, sp.batch_data_dim_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto dl_pointer = static_cast<ml::dataloaders::DataLoader<LabelType, InputType> *>(&sp);
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

    map.ExpectKeyGetValue(LABEL_SHAPE, sp.label_shape_);
    map.ExpectKeyGetValue(ONE_SAMPLE_LABEL_SHAPE, sp.one_sample_label_shape_);
    map.ExpectKeyGetValue(DATA_SHAPES, sp.data_shapes_);
    map.ExpectKeyGetValue(ONE_SAMPLE_DATA_SHAPES, sp.one_sample_data_shapes_);

    map.ExpectKeyGetValue(BATCH_LABEL_DIM, sp.batch_label_dim_);
    map.ExpectKeyGetValue(BATCH_DATA_DIM, sp.batch_data_dim_);
    sp.UpdateCursor();
  }
};

}  // namespace serializers

}  // namespace fetch
