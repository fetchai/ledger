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
#include <memory>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename LabelType, typename DataType>
class DataLoader
{
public:
  using SizeType   = fetch::math::SizeType;
  using ReturnType = std::pair<LabelType, std::vector<DataType>>;

  explicit DataLoader(bool random_mode)
    : random_mode_(random_mode)
  {}

  virtual ~DataLoader()           = default;
  virtual ReturnType    GetNext() = 0;
  virtual ReturnType    PrepareBatch(fetch::math::SizeType subset_size);
  virtual std::uint64_t Size() const   = 0;
  virtual bool          IsDone() const = 0;
  virtual void          Reset()        = 0;

protected:
  bool random_mode_ = false;

private:
  std::vector<SizeType> labels_size_vec_;
  std::vector<DataType> data_vec_;
  LabelType             labels_;
};

/**
 * Creates pair of label tensor and vector of data tensors
 * Size of each tensor is [data,subset_size], where data can have any dimensions and trailing
 * dimension is subset_size
 * @tparam LabelType
 * @tparam DataType
 * @param subset_size i.e. batch size of returned Tensors
 * @return pair of label tensor and vector of data tensors with specified batch size
 */
template <typename LabelType, typename DataType>
std::pair<LabelType, std::vector<DataType>> DataLoader<LabelType, DataType>::PrepareBatch(
    fetch::math::SizeType subset_size)
{
  if (IsDone())
  {
    Reset();
  }
  ReturnType training_pair = GetNext();

  if (data_vec_.size() != training_pair.second.size())
  {
    data_vec_.resize(training_pair.second.size());
  }

  // Prepare output data tensors
  for (SizeType i{0}; i < training_pair.second.size(); i++)
  {
    std::vector<SizeType> current_data_shape             = training_pair.second.at(i).shape();
    current_data_shape.at(current_data_shape.size() - 1) = subset_size;

    if (data_vec_.at(i).shape() != current_data_shape)
    {
      data_vec_.at(i) = DataType{current_data_shape};
    }
  }

  // Prepare output label tensor
  if (labels_size_vec_ != training_pair.first.shape())
  {
    labels_size_vec_ = training_pair.first.shape();
  }

  SizeType label_batch_dimension             = labels_size_vec_.size() - 1;
  labels_size_vec_.at(label_batch_dimension) = subset_size;
  if (labels_.shape() != labels_size_vec_)
  {
    labels_ = LabelType{labels_size_vec_};
  }

  SizeType i{0};
  do
  {
    // Fill label slice
    auto label_slice = labels_.Slice(i, label_batch_dimension);
    label_slice.Assign(training_pair.first);

    // Fill all data from data vector
    for (SizeType j{0}; j < training_pair.second.size(); j++)
    {
      // Fill data[j] slice
      auto data_slice = data_vec_.at(j).Slice(i, training_pair.second.at(j).shape().size() - 1);
      data_slice.Assign(training_pair.second.at(j));
    }

    i++;
    if (i < subset_size)
    {
      if (IsDone())
      {
        Reset();
      }
      training_pair = GetNext();
    }
  } while (i < subset_size);

  return std::make_pair(labels_, data_vec_);
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
