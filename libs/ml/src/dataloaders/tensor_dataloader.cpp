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

#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

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
OperationsCount TensorDataLoader<TensorType>::ChargeGetNext()
{
  OperationsCount cost = 0;
  // cost for copying the labels array
  OperationsCount labels_copy_cost = math::Product(one_sample_label_shape_);
  cost += labels_copy_cost;

  // cost for copying the data array
  OperationsCount data_copy_cost = 0;
  for (auto const &it : one_sample_data_shapes_)
  {
    data_copy_cost += math::Product(it);
  }
  cost += data_copy_cost;

  // cost for updating cursor
  cost += 5;
  return cost;
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
OperationsCount TensorDataLoader<TensorType>::ChargeAddData(const std::vector<TensorType> &data,
                                                            const TensorType &             labels)
{
  OperationsCount cost = 2;  // setting one_sample_label_shape
  cost += 4;                 // resizing data vector
  OperationsCount label_copy_cost = TensorType::PaddedSizeFromShape(labels.shape());
  cost += label_copy_cost;

  cost += data.size();  // loop cost
  for (SizeType i{0}; i < data.size(); i++)
  {
    cost += TensorType::PaddedSizeFromShape(data.at(i).shape());  // copying data cost
    cost += data.at(i).shape().size();                            // copying data shape cost
  }
  cost += 7;  // n_samples cost

  cost += 32;  // count of operations in UpdateRanges and UpdateCursor
  return cost;
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
OperationsCount TensorDataLoader<TensorType>::ChargeIsDone() const
{
  // cost for checking count_ is estimated as 3
  return 3;
}

template <typename TensorType>
void TensorDataLoader<TensorType>::Reset()
{
  *count_                  = 0;
  *(this->current_cursor_) = this->current_min_;
}

template <typename TensorType>
void TensorDataLoader<TensorType>::SetTestRatio(fixed_point::fp32_t new_test_ratio)
{
  test_to_train_ratio_ = new_test_ratio;
  UpdateRanges();
}

template <typename TensorType>
void TensorDataLoader<TensorType>::SetValidationRatio(fixed_point::fp32_t new_validation_ratio)
{
  validation_to_train_ratio_ = new_validation_ratio;
  UpdateRanges();
}

template <typename TensorType>
void TensorDataLoader<TensorType>::UpdateRanges()
{
  fixed_point::fp32_t test_percentage =
      fixed_point::fp32_t{1} - test_to_train_ratio_ - validation_to_train_ratio_;
  fixed_point::fp32_t validation_percentage = test_percentage + test_to_train_ratio_;

  // Define where test set starts
  test_offset_ =
      static_cast<uint32_t>(test_percentage * static_cast<fixed_point::fp32_t>(n_samples_));

  if (test_offset_ == static_cast<SizeType>(0))
  {
    test_offset_ = static_cast<SizeType>(1);
  }

  // Define where validation set starts
  validation_offset_ =
      static_cast<uint32_t>(validation_percentage * static_cast<fixed_point::fp32_t>(n_samples_));

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

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// TODO(ML-438)
// template class TensorDataLoader<math::Tensor<std::int8_t>>;
// template class TensorDataLoader<math::Tensor<std::int16_t>>;
template class TensorDataLoader<math::Tensor<std::int32_t>>;
template class TensorDataLoader<math::Tensor<std::int64_t>>;
template class TensorDataLoader<math::Tensor<float>>;
template class TensorDataLoader<math::Tensor<double>>;
template class TensorDataLoader<math::Tensor<fixed_point::fp32_t>>;
template class TensorDataLoader<math::Tensor<fixed_point::fp64_t>>;
template class TensorDataLoader<math::Tensor<fixed_point::fp128_t>>;

}  // namespace dataloaders
}  // namespace ml

}  // namespace fetch
