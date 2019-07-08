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
  using SizeVector = fetch::math::SizeVector;
  using ReturnType = std::pair<LabelType, std::vector<DataType>>;

  /**
   * Dataloaders are required to provide label and DataType shapes to the parent Dataloader
   * @param random_mode
   * @param label_shape
   * @param data_shapes
   */
  explicit DataLoader(bool random_mode)
    : random_mode_(random_mode)
  {}

  virtual ~DataLoader()           = default;
  virtual ReturnType    GetNext() = 0;
  virtual ReturnType    PrepareBatch(fetch::math::SizeType subset_size, bool &is_done_set);
  virtual std::uint64_t Size() const   = 0;
  virtual bool          IsDone() const = 0;
  virtual void          Reset()        = 0;

protected:
  bool random_mode_ = false;

private:
  bool       size_not_set_ = true;
  SizeType   label_batch_dimension_;
  SizeVector data_batch_dimensions_;
  ReturnType cur_training_pair_;

  void SetDataSize(std::pair<LabelType, std::vector<DataType>> &ret_pair);
  std::pair<LabelType, std::vector<DataType>> ret_pair_;
};

/**
 * This method sets the shapes of the data and labels, as well as the return pair.
 * @tparam LabelType
 * @tparam DataType
 * @param ret_pair
 */
template <typename LabelType, typename DataType>
void DataLoader<LabelType, DataType>::SetDataSize(
    std::pair<LabelType, std::vector<DataType>> &ret_pair)
{
  label_batch_dimension_ = ret_pair.first.shape().size() - 1;
  ret_pair_.first        = ret_pair.first.Copy();

  // set each tensor in the data vector to the correct shape
  for (auto &tensor : ret_pair.second)
  {
    data_batch_dimensions_.emplace_back(tensor.shape().size() - 1);
    ret_pair_.second.emplace_back(tensor.Copy());
  }
}

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
typename DataLoader<LabelType, DataType>::ReturnType DataLoader<LabelType, DataType>::PrepareBatch(
    fetch::math::SizeType subset_size, bool &is_done_set)
{
  //
  if (size_not_set_)
  {
    // first ever call to PrepareBatch requires a dummy GetNext to identify tensor shapes
    cur_training_pair_ = GetNext();
    Reset();

    this->SetDataSize(cur_training_pair_);
    size_not_set_ = false;
  }

  // if the label is set to be the wrong batch_size reshape
  if (ret_pair_.first.shape().at(ret_pair_.first.shape().size() - 1) != subset_size)
  {
    SizeVector new_shape               = ret_pair_.first.shape();
    new_shape.at(new_shape.size() - 1) = subset_size;
    ret_pair_.first.Reshape(new_shape);
  }

  // if the data tensor are set to be the wrong batch_size reshape
  for (SizeType tensor_count = 0; tensor_count < ret_pair_.second.size(); ++tensor_count)
  {
    // reshape the tensor to the correct batch size
    if (ret_pair_.second.at(tensor_count)
            .shape()
            .at(ret_pair_.second.at(tensor_count).shape().size() - 1) != subset_size)
    {
      SizeVector new_shape               = ret_pair_.second.at(tensor_count).shape();
      new_shape.at(new_shape.size() - 1) = subset_size;
      ret_pair_.second.at(tensor_count).Reshape(new_shape);
    }
  }

  SizeType data_idx{0};
  while (data_idx < subset_size)
  {
    // check if end of data
    if (IsDone())
    {
      is_done_set = true;
      Reset();
    }

    // get next datum & label
    cur_training_pair_ = GetNext();

    // Fill label slice
    auto label_slice = ret_pair_.first.Slice(data_idx, label_batch_dimension_);
    label_slice.Assign(cur_training_pair_.first);

    // Fill all data from data vector
    for (SizeType j{0}; j < cur_training_pair_.second.size(); j++)
    {
      // Fill data[j] slice
      auto data_slice = ret_pair_.second.at(j).Slice(data_idx, data_batch_dimensions_.at(j));
      data_slice.Assign(cur_training_pair_.second.at(j));
    }

    // increment the count
    ++data_idx;
  }
  return ret_pair_;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
