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
#include "math/base_types.hpp"
#include "ml/exceptions/exceptions.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

enum class DataLoaderMode
{
  TRAIN,
  VALIDATE,
  TEST,
};

template <typename LabelType, typename InputType>
class DataLoader
{
public:
  using SizeType   = fetch::math::SizeType;
  using SizeVector = fetch::math::SizeVector;
  using ReturnType = std::pair<LabelType, std::vector<InputType>>;

  explicit DataLoader() = default;

  virtual ~DataLoader() = default;

  virtual ReturnType GetNext() = 0;

  virtual bool       AddData(InputType const &data, LabelType const &label) = 0;
  virtual ReturnType PrepareBatch(fetch::math::SizeType batch_size, bool &is_done_set);

  virtual SizeType Size() const                                   = 0;
  virtual bool     IsDone() const                                 = 0;
  virtual void     Reset()                                        = 0;
  virtual void     SetTestRatio(float new_test_ratio)             = 0;
  virtual void     SetValidationRatio(float new_validation_ratio) = 0;
  void             SetMode(DataLoaderMode new_mode);
  virtual bool     IsModeAvailable(DataLoaderMode mode) = 0;
  void             SetRandomMode(bool random_mode_state);

  template <typename X, typename D>
  friend struct fetch::serializers::MapSerializer;

protected:
  virtual void              UpdateCursor() = 0;
  std::shared_ptr<SizeType> current_cursor_;
  SizeType                  current_min_{};
  SizeType                  current_max_{};
  SizeType                  current_size_{};

  bool           random_mode_ = false;
  DataLoaderMode mode_        = DataLoaderMode::TRAIN;

private:
  bool       size_not_set_ = true;
  ReturnType cur_training_pair_;
  ReturnType ret_pair_;

  void SetDataSize(std::pair<LabelType, std::vector<InputType>> &ret_pair);
};

/**
 * This method sets the shapes of the data and labels, as well as the return pair.
 * @tparam LabelType
 * @tparam InputType
 * @param ret_pair
 */
template <typename LabelType, typename InputType>
void DataLoader<LabelType, InputType>::SetDataSize(
    std::pair<LabelType, std::vector<InputType>> &ret_pair)
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
 * @tparam InputType
 * @param batch_size i.e. batch size of returned Tensors
 * @return pair of label tensor and vector of data tensors with specified batch size
 */
template <typename LabelType, typename InputType>
typename DataLoader<LabelType, InputType>::ReturnType
DataLoader<LabelType, InputType>::PrepareBatch(fetch::math::SizeType batch_size, bool &is_done_set)
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

template <typename LabelType, typename DataType>
void DataLoader<LabelType, DataType>::SetMode(DataLoaderMode new_mode)
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

template <typename LabelType, typename DataType>
void DataLoader<LabelType, DataType>::SetRandomMode(bool random_mode_state)
{
  random_mode_ = random_mode_state;
}

}  // namespace dataloaders
}  // namespace ml

namespace serializers {

/**
 * serializer for Dataloader
 * @tparam TensorType
 */
template <typename LabelType, typename InputType, typename D>
struct MapSerializer<fetch::ml::dataloaders::DataLoader<LabelType, InputType>, D>
{
  using Type       = fetch::ml::dataloaders::DataLoader<LabelType, InputType>;
  using DriverType = D;

  static uint8_t const RANDOM_MODE              = 1;
  static uint8_t const SIZE_NOT_SET             = 2;
  static uint8_t const CUR_TRAINING_PAIR_FIRST  = 3;
  static uint8_t const CUR_TRAINING_PAIR_SECOND = 4;
  static uint8_t const RET_PAIR_FIRST           = 5;
  static uint8_t const RET_PAIR_SECOND          = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);

    map.Append(RANDOM_MODE, sp.random_mode_);
    map.Append(SIZE_NOT_SET, sp.size_not_set_);
    map.Append(CUR_TRAINING_PAIR_FIRST, sp.cur_training_pair_.first);
    map.Append(CUR_TRAINING_PAIR_SECOND, sp.cur_training_pair_.second);
    map.Append(RET_PAIR_FIRST, sp.ret_pair_.first);
    map.Append(RET_PAIR_SECOND, sp.ret_pair_.second);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(RANDOM_MODE, sp.random_mode_);
    map.ExpectKeyGetValue(SIZE_NOT_SET, sp.size_not_set_);
    map.ExpectKeyGetValue(CUR_TRAINING_PAIR_FIRST, sp.cur_training_pair_.first);
    map.ExpectKeyGetValue(CUR_TRAINING_PAIR_SECOND, sp.cur_training_pair_.second);
    map.ExpectKeyGetValue(RET_PAIR_FIRST, sp.ret_pair_.first);
    map.ExpectKeyGetValue(RET_PAIR_SECOND, sp.ret_pair_.second);
  }
};

}  // namespace serializers

}  // namespace fetch
