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

#include "core/serializers/group_definitions.hpp"
#include "ml/dataloaders/dataloader.hpp"

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
  TensorDataLoader(SizeVector const &label_shape, std::vector<SizeVector> const &data_shapes,
                   bool random_mode = false, float VALIDATION_TO_TRAIN_RATIO = 0.0);

  ~TensorDataLoader() override = default;

  ReturnType   GetNext() override;
  virtual bool AddData(TensorType const &data, TensorType const &labels);

  SizeType    Size() const override;
  bool        IsDone() const override;
  void        Reset() override;
  inline bool IsValidable() const override;

  template <typename X, typename D>
  friend struct fetch::serializers::MapSerializer;

protected:
  SizeType train_cursor_      = 0;
  SizeType validation_cursor_ = 0;
  SizeType validation_offset_ = 0;

  SizeType n_samples_            = 0;  // number of all samples
  SizeType n_validation_samples_ = 0;  // number of train samples
  SizeType n_train_samples_      = 0;  // number of validation samples

  TensorType data_;
  TensorType labels_;

  SizeVector              label_shape_;
  SizeVector              one_sample_label_shape_;
  std::vector<SizeVector> data_shapes_;
  std::vector<SizeVector> one_sample_data_shapes_;
  float                   validation_to_train_ratio_ = 0.0;

  SizeType batch_label_dim_ = fetch::math::numeric_max<SizeType>();
  SizeType batch_data_dim_  = fetch::math::numeric_max<SizeType>();

  void UpdateCursor() override;
};

template <typename LabelType, typename InputType>
TensorDataLoader<LabelType, InputType>::TensorDataLoader(SizeVector const &             label_shape,
                                                         std::vector<SizeVector> const &data_shapes,
                                                         bool                           random_mode,
                                                         float validation_to_train_ratio)
  : DataLoader<LabelType, TensorType>(random_mode, DataLoaderMode::TRAIN)
  , label_shape_(label_shape)
  , data_shapes_(data_shapes)
  , validation_to_train_ratio_(validation_to_train_ratio)
{
  one_sample_label_shape_                                        = label_shape;
  one_sample_label_shape_.at(one_sample_label_shape_.size() - 1) = 1;

  for (std::size_t i = 0; i < data_shapes.size(); ++i)
  {
    one_sample_data_shapes_.emplace_back(data_shapes.at(i));
    one_sample_data_shapes_.at(i).at(one_sample_data_shapes_.at(i).size() - 1) = 1;
  }
}

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::ReturnType
TensorDataLoader<LabelType, InputType>::GetNext()
{
  if (this->random_mode_)
  {
    throw std::runtime_error("random mode not implemented for tensor dataloader");
  }

  if (this->mode_ == DataLoaderMode::VALIDATE)
  {
    ReturnType ret(labels_.View(validation_cursor_).Copy(one_sample_label_shape_),
                   {data_.View(validation_cursor_).Copy(one_sample_data_shapes_.at(0))});
    validation_cursor_++;
    return ret;
  }
  else
  {
    ReturnType ret(labels_.View(train_cursor_).Copy(one_sample_label_shape_),
                   {data_.View(train_cursor_).Copy(one_sample_data_shapes_.at(0))});
    train_cursor_++;
    return ret;
  }
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::AddData(TensorType const &data,
                                                     TensorType const &labels)
{
  data_         = data.Copy();
  labels_       = labels.Copy();
  train_cursor_ = 0;

  batch_label_dim_ = labels_.shape().size() - 1;
  batch_data_dim_  = data_.shape().size() - 1;

  n_samples_ = data_.shape().at(batch_data_dim_);

  n_validation_samples_ =
      static_cast<SizeType>(validation_to_train_ratio_ * static_cast<float>(n_samples_));
  n_train_samples_ = n_samples_ - n_validation_samples_;

  validation_offset_ = n_train_samples_;

  return true;
}

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::SizeType
TensorDataLoader<LabelType, InputType>::Size() const
{
  if (this->mode_ == DataLoaderMode::VALIDATE)
  {
    return n_validation_samples_;
  }
  else
  {
    return n_train_samples_;
  }
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::IsDone() const
{
  if (this->mode_ == DataLoaderMode::TRAIN)
  {
    return (train_cursor_ >= n_train_samples_);
  }
  else if (this->mode_ == DataLoaderMode::VALIDATE)
  {
    return (validation_cursor_ >= n_validation_samples_);
  }
  else
  {
    throw std::runtime_error("Other modes than TRAIN and VALIDATE not supported.");
  }
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::Reset()
{
  if (this->mode_ == DataLoaderMode::TRAIN)
  {
    train_cursor_ = 0;
  }
  else if (this->mode_ == DataLoaderMode::VALIDATE)
  {
    validation_cursor_ = 0;
  }
  else
  {
    throw std::runtime_error("Other modes than TRAIN and VALIDATE not supported.");
  }
}

template <typename LabelType, typename InputType>
inline bool TensorDataLoader<LabelType, InputType>::IsValidable() const
{
  return n_validation_samples_ > 0;
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::UpdateCursor()
{
  if (this->mode_ != DataLoaderMode::TRAIN)
  {
    throw std::runtime_error("Other mode than training not supported yet.");
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
  static uint8_t const VALIDATION_CURSOR         = 2;
  static uint8_t const TRAIN_CURSOR              = 3;
  static uint8_t const VALIDATION_OFFSET         = 4;
  static uint8_t const VALIDATION_TO_TRAIN_RATIO = 5;
  static uint8_t const N_SAMPLES                 = 6;
  static uint8_t const N_TRAIN_SAMPLES           = 7;
  static uint8_t const N_VALIDATION_SAMPLES      = 8;

  static uint8_t const DATA   = 9;
  static uint8_t const LABELS = 10;

  static uint8_t const LABEL_SHAPE            = 11;
  static uint8_t const ONE_SAMPLE_LABEL_SHAPE = 12;
  static uint8_t const DATA_SHAPES            = 13;
  static uint8_t const ONE_SAMPLE_DATA_SHAPES = 14;

  static uint8_t const BATCH_LABEL_DIM = 15;
  static uint8_t const BATCH_DATA_DIM  = 16;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(16);

    // serialize parent class first
    auto dl_pointer = static_cast<ml::dataloaders::DataLoader<LabelType, InputType> const *>(&sp);
    map.Append(BASE_DATA_LOADER, *(dl_pointer));

    map.Append(VALIDATION_CURSOR, sp.validation_cursor_);
    map.Append(TRAIN_CURSOR, sp.train_cursor_);
    map.Append(VALIDATION_OFFSET, sp.validation_offset_);
    map.Append(VALIDATION_TO_TRAIN_RATIO, sp.validation_to_train_ratio_);
    map.Append(N_SAMPLES, sp.n_samples_);
    map.Append(N_TRAIN_SAMPLES, sp.n_train_samples_);
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

    map.ExpectKeyGetValue(VALIDATION_CURSOR, sp.validation_cursor_);
    map.ExpectKeyGetValue(TRAIN_CURSOR, sp.train_cursor_);
    map.ExpectKeyGetValue(VALIDATION_OFFSET, sp.validation_offset_);
    map.ExpectKeyGetValue(VALIDATION_TO_TRAIN_RATIO, sp.validation_to_train_ratio_);
    map.ExpectKeyGetValue(N_SAMPLES, sp.n_samples_);
    map.ExpectKeyGetValue(N_TRAIN_SAMPLES, sp.n_train_samples_);
    map.ExpectKeyGetValue(N_VALIDATION_SAMPLES, sp.n_validation_samples_);

    map.ExpectKeyGetValue(DATA, sp.data_);
    map.ExpectKeyGetValue(LABELS, sp.labels_);

    map.ExpectKeyGetValue(LABEL_SHAPE, sp.label_shape_);
    map.ExpectKeyGetValue(ONE_SAMPLE_LABEL_SHAPE, sp.one_sample_label_shape_);
    map.ExpectKeyGetValue(DATA_SHAPES, sp.data_shapes_);
    map.ExpectKeyGetValue(ONE_SAMPLE_DATA_SHAPES, sp.one_sample_data_shapes_);

    map.ExpectKeyGetValue(BATCH_LABEL_DIM, sp.batch_label_dim_);
    map.ExpectKeyGetValue(BATCH_DATA_DIM, sp.batch_data_dim_);
  }
};

}  // namespace serializers

}  // namespace fetch
