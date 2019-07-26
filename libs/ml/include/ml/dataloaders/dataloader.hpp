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

#include <cstdint>
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

  virtual ~DataLoader() = default;

  virtual ReturnType GetNext() = 0;

  virtual ReturnType PrepareBatch(fetch::math::SizeType subset_size, bool &is_done_set);

  virtual std::uint64_t Size() const   = 0;
  virtual bool          IsDone() const = 0;
  virtual void          Reset()        = 0;

  template <typename S>
  friend void Serialize(S &serializer, DataLoader<LabelType, DataType> const &dl)
  {
    serializer << dl.random_mode_;
    serializer << dl.size_not_set_;

    Serialize(serializer, dl.cur_training_pair_.first);
    for (auto &val : dl.cur_training_pair_.second)
    {
      serializer << val;
    }

    Serialize(serializer, dl.ret_pair_.first);
    for (auto &val : dl.ret_pair_.second)
    {
      serializer << val;
    }
  }

  template <typename S>
  friend void Deserialize(S &serializer, DataLoader<LabelType, DataType> &dl)
  {
    serializer >> dl.random_mode_;
    serializer >> dl.size_not_set_;

    Deserialize(serializer, dl.cur_training_pair_.first);
    for (auto &val : dl.cur_training_pair_.second)
    {
      serializer >> val;
    }

    Deserialize(serializer, dl.ret_pair_.first);
    for (auto &val : dl.ret_pair_.second)
    {
      serializer >> val;
    }
  }

protected:
  bool random_mode_ = false;

private:
  bool       size_not_set_ = true;
  ReturnType cur_training_pair_;
  ReturnType ret_pair_;

  void SetDataSize(std::pair<LabelType, std::vector<DataType>> &ret_pair);
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
  ret_pair_.first = ret_pair.first.Copy();

  // set each tensor in the data vector to the correct shape
  for (auto &tensor : ret_pair.second)
  {
    ret_pair_.second.emplace_back(tensor.Copy());
  }
}

/**
 * Creates pair of label tensor and vector of data tensors
 * Size of each tensor is [data,subset_size], where data can have any dimensions and trailing
 * dimension is subset_size
 * @tparam LabelType
 * @tparam DataType
 * @param batch_size i.e. batch size of returned Tensors
 * @return pair of label tensor and vector of data tensors with specified batch size
 */
template <typename LabelType, typename DataType>
typename DataLoader<LabelType, DataType>::ReturnType DataLoader<LabelType, DataType>::PrepareBatch(
    fetch::math::SizeType batch_size, bool &is_done_set)
{
  if (size_not_set_)
  {
    // first ever call to PrepareBatch requires a dummy GetNext to identify tensor shapes
    cur_training_pair_ = GetNext();
    Reset();

    this->SetDataSize(cur_training_pair_);
    size_not_set_ = false;
  }

  // if the label is set to be the wrong batch_size reshape
  if (ret_pair_.first.shape().at(ret_pair_.first.shape().size() - 1) != batch_size)
  {
    SizeVector new_shape               = ret_pair_.first.shape();
    new_shape.at(new_shape.size() - 1) = batch_size;
    ret_pair_.first.Reshape(new_shape);
  }

  // if the data tensor are set to be the wrong batch_size reshape
  for (SizeType tensor_count = 0; tensor_count < ret_pair_.second.size(); ++tensor_count)
  {
    // reshape the tensor to the correct batch size
    if (ret_pair_.second.at(tensor_count)
            .shape()
            .at(ret_pair_.second.at(tensor_count).shape().size() - 1) != batch_size)
    {
      SizeVector new_shape               = ret_pair_.second.at(tensor_count).shape();
      new_shape.at(new_shape.size() - 1) = batch_size;
      ret_pair_.second.at(tensor_count).Reshape(new_shape);
    }
  }

  SizeType data_idx{0};
  while (data_idx < batch_size)
  {
    // check if end of data
    if (IsDone())
    {
      is_done_set = true;
      Reset();
    }

    // get next datum & label
    cur_training_pair_ = GetNext();

    // Fill label view
    auto label_view = ret_pair_.first.View(data_idx);
    label_view.Assign(cur_training_pair_.first);

    // Fill all data from data vector
    for (SizeType j{0}; j < cur_training_pair_.second.size(); j++)
    {
      // Fill data[j] view
      auto data_view = ret_pair_.second.at(j).View(data_idx);
      data_view.Assign(cur_training_pair_.second.at(j));
    }

    // increment the count
    ++data_idx;
  }
  return ret_pair_;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
