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

#include "core/random.hpp"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/dataloader.hpp"

#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename LabelType, typename InputType>
class CommodityDataLoader : public DataLoader<LabelType, InputType>
{
public:
  using DataType   = typename InputType::Type;
  using ReturnType = std::pair<LabelType, std::vector<InputType>>;
  using SizeType   = math::SizeType;

  explicit CommodityDataLoader(bool random_mode = false)
    : DataLoader<LabelType, InputType>(random_mode)
  {}

  ~CommodityDataLoader() override = default;

  ReturnType  GetNext() override;
  SizeType    Size() const override;
  bool        IsDone() const override;
  void        Reset() override;
  inline bool IsValidable() const override;

  bool AddData(InputType const &data, LabelType const &label) override;

private:
  bool      random_mode_ = false;
  InputType data_;    // n_data, features
  LabelType labels_;  // n_data, features

  SizeType   rows_to_skip_ = 1;
  SizeType   cols_to_skip_ = 1;
  ReturnType buffer_;
  SizeType   cursor_ = 0;
  SizeType   size_   = 0;

  random::Random rand;

  void GetAtIndex(SizeType index);
  void UpdateCursor() override;
};

/**
 * Adds input data and labels of commodity data.
 * By default this skips the first row and column as these are assumed to be indices
 * @tparam LabelType
 * @tparam InputType
 * @param data
 * @param label
 * @return
 */
template <typename LabelType, typename InputType>
bool CommodityDataLoader<LabelType, InputType>::AddData(InputType const &data,
                                                        LabelType const &label)
{
  data_   = data;
  labels_ = label;
  assert(data_.shape().at(1) == labels_.shape().at(1));
  size_ = data.shape().at(1);

  return true;
}

/**
 * Gets the next pair of data and labels
 * @tparam LabelType type for the labels
 * @tparam InputType type for the data
 * @return Pair of input and output
 */
template <typename LabelType, typename InputType>
typename CommodityDataLoader<LabelType, InputType>::ReturnType
CommodityDataLoader<LabelType, InputType>::GetNext()
{
  if (this->random_mode_)
  {
    GetAtIndex(static_cast<SizeType>(decltype(rand)::generator()) % Size());
    return buffer_;
  }
  else
  {
    GetAtIndex(static_cast<SizeType>(cursor_++));
    return buffer_;
  }
}

/**
 * Returns the number of datapoints
 * @return number of datapoints
 */
template <typename LabelType, typename InputType>
typename CommodityDataLoader<LabelType, InputType>::SizeType
CommodityDataLoader<LabelType, InputType>::Size() const
{
  return static_cast<SizeType>(size_);
}

template <typename LabelType, typename InputType>
bool CommodityDataLoader<LabelType, InputType>::IsDone() const
{
  return cursor_ >= size_;
}

/**
 * Resets the cursor to 0
 */
template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::Reset()
{
  cursor_ = 0;
}

template <typename LabelType, typename InputType>
inline bool CommodityDataLoader<LabelType, InputType>::IsValidable() const
{
  // Validation set splitting not implemented yet
  return false;
}

/**
 * Returns the data pair at the given index
 * @param index index of the data
 */
template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::GetAtIndex(CommodityDataLoader::SizeType index)
{
  buffer_.first  = labels_.View(index).Copy();
  buffer_.second = std::vector<InputType>({data_.View(index).Copy()});
}

template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::UpdateCursor()
{
  if (this->mode_ != DataLoaderMode::TRAIN)
  {
    throw std::runtime_error("Other mode than training not supported yet.");
  }
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
