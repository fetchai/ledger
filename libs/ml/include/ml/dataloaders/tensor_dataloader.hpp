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
  TensorDataLoader(SizeVector const &label_shape, std::vector<SizeVector> const &data_shapes,
                   bool random_mode = false, float test_to_train_ratio = 0.0)
    : DataLoader<LabelType, TensorType>(random_mode)
    , label_shape_(label_shape)
    , data_shapes_(data_shapes)
    , test_to_train_ratio_(test_to_train_ratio)
  {
    one_sample_label_shape_                                        = label_shape;
    one_sample_label_shape_.at(one_sample_label_shape_.size() - 1) = 1;

    for (std::size_t i = 0; i < data_shapes.size(); ++i)
    {
      one_sample_data_shapes_.emplace_back(data_shapes.at(i));
      one_sample_data_shapes_.at(i).at(one_sample_data_shapes_.at(i).size() - 1) = 1;
    }
  }

  ~TensorDataLoader() override = default;

  ReturnType   GetNext(bool is_test = false) override;
  virtual bool AddData(TensorType const &data, TensorType const &labels);

  SizeType Size(bool is_test = false) const override;
  bool     IsDone(bool is_test = false) const override;
  void     Reset(bool is_test = false) override;

protected:
  SizeType train_cursor_ = 0;
  SizeType test_cursor_  = 0;
  SizeType test_offset_  = 0;

  SizeType n_samples_       = 0;  // number of all samples
  SizeType n_test_samples_  = 0;  // number of train samples
  SizeType n_train_samples_ = 0;  // number of test samples

  TensorType data_;
  TensorType labels_;

  SizeVector              label_shape_;
  SizeVector              one_sample_label_shape_;
  std::vector<SizeVector> data_shapes_;
  std::vector<SizeVector> one_sample_data_shapes_;
  float                   test_to_train_ratio_ = 0.0;

  SizeType batch_label_dim_ = fetch::math::numeric_max<SizeType>();
  SizeType batch_data_dim_  = fetch::math::numeric_max<SizeType>();
};

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::ReturnType
TensorDataLoader<LabelType, InputType>::GetNext(bool is_test)
{
  if (this->random_mode_)
  {
    throw std::runtime_error("random mode not implemented for tensor dataloader");
  }

  if (is_test)
  {
    ReturnType ret(labels_.View(test_cursor_).Copy(one_sample_label_shape_),
                   {data_.View(test_cursor_).Copy(one_sample_data_shapes_.at(0))});
    test_cursor_++;
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

  n_test_samples_  = static_cast<SizeType>(test_to_train_ratio_ * static_cast<float>(n_samples_));
  n_train_samples_ = n_samples_ - n_test_samples_;

  test_offset_ = n_test_samples_;

  return true;
}

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::SizeType
TensorDataLoader<LabelType, InputType>::Size(bool is_test) const
{
  if (is_test)
  {
    return n_test_samples_;
  }
  else
  {
    return n_train_samples_;
  }
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::IsDone(bool is_test) const
{
  if (is_test)
  {
    throw std::runtime_error("Validation set splitting not implemented yet");
  }

  if (is_test)
  {
    return (train_cursor_ >= n_train_samples_);
  }
  else
  {
    return (test_cursor_ >= test_offset_ + n_test_samples_);
  }
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::Reset(bool is_test)
{
  if (is_test)
  {
    test_cursor_ = 0;
  }
  else
  {
    train_cursor_ = 0;
  }
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
