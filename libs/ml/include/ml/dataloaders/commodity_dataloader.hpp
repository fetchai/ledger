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

#include <fstream>
#include <vector>

#include "core/random.hpp"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/dataloader.hpp"

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
  {
    // TODO (1314) - make prepare batch compliant
    // prepares underlying containers for buffering data and labels
    // this->SetDataSize({label_shape}, {{t1_shape}, {t2_shape}, ...});
  }

  ~CommodityDataLoader() = default;

  virtual ReturnType GetNext();

  virtual SizeType Size() const;

  virtual bool IsDone() const;

  virtual void Reset();

  void AddData(std::string const &xfilename, std::string const &yfilename);

private:
  bool      random_mode_ = false;
  InputType data_;    // n_data, features
  InputType labels_;  // n_data, features

  SizeType   rows_to_skip_ = 1;
  SizeType   cols_to_skip_ = 1;
  ReturnType buffer_;
  SizeType   cursor_ = 0;
  SizeType   size_   = 0;

  random::Random rand;

  void GetAtIndex(SizeType index);
};

/**
 * Adds X and Y data to the dataloader.
 * By default this skips the first row amd columns as these are assumed to be indices.
 * @param xfilename Name of the csv file with inputs
 * @param yfilename name of the csv file with labels
 */
template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::AddData(std::string const &xfilename,
                                                        std::string const &yfilename)
{
  data_   = ReadCSV<InputType>(xfilename, cols_to_skip_, rows_to_skip_, true);
  labels_ = ReadCSV<InputType>(yfilename, cols_to_skip_, rows_to_skip_, true);

  assert(data_.shape()[1] == labels_.shape()[1]);
  // save the number of datapoints
  size_ = data_.shape()[1];
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
    GetAtIndex(static_cast<SizeType>(rand.generator()) % Size());
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

/**
 * Returns the data pair at the given index
 * @param index index of the data
 */
template <typename LabelType, typename InputType>
void CommodityDataLoader<LabelType, InputType>::GetAtIndex(CommodityDataLoader::SizeType index)
{
  buffer_.first  = labels_.Slice(index, 1).Copy();
  buffer_.second = std::vector<InputType>({data_.Slice(index, 1).Copy()});
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
