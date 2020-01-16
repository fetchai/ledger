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

#include "ml/dataloaders/dataloader.hpp"

#include "math/tensor/tensor.hpp"
#include "ml/exceptions/exceptions.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * This method sets the shapes of the data and labels, as well as the return pair.
 * @tparam TensorType
 * @param ret_pair
 */
template <typename TensorType>
void DataLoader<TensorType>::SetDataSize(std::pair<TensorType, std::vector<TensorType>> &ret_pair)
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
 * @tparam TensorType
 * @param batch_size i.e. batch size of returned Tensors
 * @return pair of label tensor and vector of data tensors with specified batch size
 */
template <typename TensorType>
typename DataLoader<TensorType>::ReturnType DataLoader<TensorType>::PrepareBatch(
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

template <typename TensorType>
void DataLoader<TensorType>::SetMode(DataLoaderMode new_mode)
{
  if (mode_ == new_mode)
  {
    return;
  }
  mode_ = new_mode;
  UpdateCursor();

  if (this->current_min_ == this->current_max_)
  {
    throw exceptions::Timeout("Dataloader has no set for selected mode.");
  }
}

template <typename TensorType>
void DataLoader<TensorType>::SetRandomMode(bool random_mode_state)
{
  random_mode_ = random_mode_state;
}

template <typename TensorType>
void DataLoader<TensorType>::SetSeed(DataLoader::SizeType seed)
{
  rand.Seed(seed);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class DataLoader<math::Tensor<std::int8_t>>;
template class DataLoader<math::Tensor<std::int16_t>>;
template class DataLoader<math::Tensor<std::int32_t>>;
template class DataLoader<math::Tensor<std::int64_t>>;
template class DataLoader<math::Tensor<float>>;
template class DataLoader<math::Tensor<double>>;
template class DataLoader<math::Tensor<fixed_point::fp32_t>>;
template class DataLoader<math::Tensor<fixed_point::fp64_t>>;
template class DataLoader<math::Tensor<fixed_point::fp128_t>>;

}  // namespace dataloaders
}  // namespace ml

}  // namespace fetch
