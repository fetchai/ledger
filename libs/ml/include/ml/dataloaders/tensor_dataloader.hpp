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
                   bool random_mode = false)
    : DataLoader<LabelType, TensorType>(random_mode)
    , label_shape_(label_shape)
    , data_shapes_(data_shapes)
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

  ReturnType   GetNext() override;
  virtual bool AddData(TensorType const &data, TensorType const &labels);

  SizeType Size() const override;
  bool     IsDone() const override;
  void     Reset() override;

  ////////////////////////////////
  /// Serialization operations ///
  ////////////////////////////////

  template <typename S>
  friend void Serialize(S &serializer, TensorDataLoader<LabelType, InputType> const &dl)
  {
    serializer << dl.data_cursor_;
    serializer << dl.label_cursor_;
    serializer << dl.n_samples_;

    serializer << dl.data_;
    serializer << dl.labels_;

    serializer << dl.label_shape_;
    serializer << dl.one_sample_label_shape_;
    serializer << dl.data_shapes_;
    serializer << dl.one_sample_data_shapes_;

    serializer << dl.batch_label_dim_;
    serializer << dl.batch_data_dim_;
  }

  template <typename S>
  friend void Deserialize(S &serializer, TensorDataLoader<LabelType, InputType> &dl)
  {

    serializer >> dl.data_cursor_;
    serializer >> dl.label_cursor_;
    serializer >> dl.n_samples_;

    serializer >> dl.data_;
    serializer >> dl.labels_;

    serializer >> dl.label_shape_;
    serializer >> dl.one_sample_label_shape_;
    serializer >> dl.data_shapes_;
    serializer >> dl.one_sample_data_shapes_;

    serializer >> dl.batch_label_dim_;
    serializer >> dl.batch_data_dim_;
  }

protected:
  SizeType data_cursor_  = 0;
  SizeType label_cursor_ = 0;
  SizeType n_samples_    = 0;  // number of data samples

  TensorType data_;
  TensorType labels_;

  SizeVector              label_shape_;
  SizeVector              one_sample_label_shape_;
  std::vector<SizeVector> data_shapes_;
  std::vector<SizeVector> one_sample_data_shapes_;

  SizeType batch_label_dim_ = fetch::math::numeric_max<SizeType>();
  SizeType batch_data_dim_  = fetch::math::numeric_max<SizeType>();
};

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::ReturnType
TensorDataLoader<LabelType, InputType>::GetNext()
{
  if (this->random_mode_)
  {
    throw std::runtime_error("random mode not implemented for tensor dataloader");
  }
  else
  {
    ReturnType ret(labels_.View(label_cursor_).Copy(one_sample_label_shape_),
                   {data_.View(data_cursor_).Copy(one_sample_data_shapes_.at(0))});
    data_cursor_++;
    label_cursor_++;
    return ret;
  }
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::AddData(TensorType const &data,
                                                     TensorType const &labels)
{

  data_         = data.Copy();
  labels_       = labels.Copy();
  data_cursor_  = 0;
  label_cursor_ = 0;

  n_samples_ = data_.shape().at(data_.shape().size() - 1);

  batch_label_dim_ = labels_.shape().size() - 1;
  batch_data_dim_  = data_.shape().size() - 1;

  return true;
}

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::SizeType
TensorDataLoader<LabelType, InputType>::Size() const
{
  return n_samples_;
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::IsDone() const
{
  return (data_cursor_ >= data_.shape(batch_data_dim_));
}

template <typename LabelType, typename InputType>
void TensorDataLoader<LabelType, InputType>::Reset()
{
  data_cursor_  = 0;
  label_cursor_ = 0;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
