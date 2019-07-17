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

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename LabelType, typename InputType>
class TensorDataLoader : public DataLoader<LabelType, InputType>
{
  using TensorType = InputType;
  using DataType   = typename TensorType::Type;

  using SizeType     = fetch::math::SizeType;
  using ReturnType   = std::pair<LabelType, std::vector<TensorType>>;
  using IteratorType = typename TensorType::IteratorType;

public:
  TensorDataLoader(bool random_mode = false)
    : DataLoader<LabelType, TensorType>(random_mode){};
  virtual ~TensorDataLoader() = default;

  virtual ReturnType GetNext();
  virtual bool       AddData(TensorType const &data, TensorType const &labels);

  virtual SizeType Size() const;
  virtual bool     IsDone() const;
  virtual void     Reset();

protected:
  SizeType data_cursor_;
  SizeType label_cursor_;

  TensorType data_;
  TensorType labels_;
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
    ReturnType ret(labels_.Slice(label_cursor_, 1).Copy(), {data_.Slice(label_cursor_, 1).Copy()});
    data_cursor_++;
    label_cursor_++;
    return ret;
  }
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::AddData(TensorType const &data,
                                                     TensorType const &labels)
{
  assert(data.shape().size() == 2);
  assert(labels.shape().size() == 2);
  data_         = data.Copy();
  labels_       = labels.Copy();
  data_cursor_  = 0;
  label_cursor_ = 0;

  return true;
}

template <typename LabelType, typename InputType>
typename TensorDataLoader<LabelType, InputType>::SizeType
TensorDataLoader<LabelType, InputType>::Size() const
{
  return data_.size();
}

template <typename LabelType, typename InputType>
bool TensorDataLoader<LabelType, InputType>::IsDone() const
{
  return (data_cursor_ >= data_.shape()[0]);
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
