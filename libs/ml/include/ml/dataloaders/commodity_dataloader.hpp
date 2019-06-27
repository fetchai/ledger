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
#include "ml/dataloaders/dataloader.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

inline std::pair<math::SizeType, math::SizeType> count_rows_cols(std::string const &filename)
{
  // find number of rows and columns in the file
  std::ifstream  file(filename);
  std::string    buf;
  std::string    delimiter = ",";
  math::SizeType pos;
  math::SizeType row{0};
  math::SizeType col{0};
  while (std::getline(file, buf, '\n'))
  {
    while ((row == 0) && ((pos = buf.find(delimiter)) != std::string::npos))
    {
      buf.erase(0, pos + delimiter.length());
      ++col;
    }
    ++row;
  }
  return std::make_pair(row, col + 1);
}

template <typename LabelType, typename InputType>
class CommodityDataLoader : DataLoader<LabelType, InputType>
{
public:
  using DataType   = typename InputType::Type;
  using ReturnType = std::pair<LabelType, std::vector<InputType>>;
  using SizeType   = math::SizeType;

  explicit CommodityDataLoader(bool random_mode = false);

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
  std::pair<SizeType, SizeType> xshape = count_rows_cols(xfilename);
  std::pair<SizeType, SizeType> yshape = count_rows_cols(yfilename);
  SizeType                      row    = xshape.first;
  SizeType                      col    = xshape.second;

  data_.Reshape({row - rows_to_skip_, col - cols_to_skip_});
  labels_.Reshape({yshape.first - rows_to_skip_, yshape.second - cols_to_skip_});
  assert(xshape.first == yshape.first);
  // save the number of rows in the data
  size_ = row - rows_to_skip_;

  // read csv data into buffer_ array
  std::ifstream file(xfilename);
  std::string   buf;
  char          delimiter = ',';
  std::string   field_value;

  for (SizeType i = 0; i < rows_to_skip_; i++)
  {
    std::getline(file, buf, '\n');
  }

  row = 0;
  while (std::getline(file, buf, '\n'))
  {
    col = 0;
    std::stringstream ss(buf);
    for (SizeType i = 0; i < cols_to_skip_; i++)
    {
      std::getline(ss, field_value, delimiter);
    }

    while (std::getline(ss, field_value, delimiter))
    {
      data_(row, col) = static_cast<DataType>(stod(field_value));
      ++col;
    }
    ++row;
  }

  file.close();

  file.open(yfilename);
  for (SizeType i = 0; i < rows_to_skip_; i++)
  {
    std::getline(file, buf, '\n');
  }

  row = 0;
  while (std::getline(file, buf, '\n'))
  {
    col = 0;
    std::stringstream ss(buf);
    for (SizeType i = 0; i < cols_to_skip_; i++)
    {
      std::getline(ss, field_value, delimiter);
    }

    while (std::getline(ss, field_value, delimiter))
    {
      labels_(row, col) = static_cast<DataType>(stod(field_value));
      ++col;
    }
    ++row;
  }
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
 * Constructor for CommodityDataLoader
 * @tparam LabelType Type for the labels, usually tensor<float>
 * @tparam InputType Type for the data, usually tensor<float>
 * @param random_mode whether to access the data randomly
 */
template <typename LabelType, typename InputType>
CommodityDataLoader<LabelType, InputType>::CommodityDataLoader(bool random_mode)
  : DataLoader<LabelType, InputType>(random_mode)
{}

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
  buffer_.first  = labels_.Slice(index).Copy();
  buffer_.second = std::vector<InputType>({data_.Slice(index).Copy()});
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
