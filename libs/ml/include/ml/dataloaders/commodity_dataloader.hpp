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

/**
 * Loads a csv file into an ArrayType (a Tensor)
 * The Tensor will have the same number of rows as this file has (minus rows_to_skip) and the same
 * number of columns (minus cols_to_skip) as the file, unless transpose=true is specified in which
 * case it will be transposed.
 * @param filename  name of the file
 * @param cols_to_skip  number of columns to skip
 * @param rows_to_skip  number of rows to skip
 * @param transpose  whether to transpose the resulting Tensor
 * @return ArrayType with data
 */
template <typename ArrayType>
ArrayType ReadCSV(std::string const &filename, math::SizeType const cols_to_skip = 0,
                  math::SizeType rows_to_skip = 0, bool transpose = false)
{
  using DataType = typename ArrayType::Type;
  std::ifstream file(filename);
  if (file.fail())
  {
    throw std::runtime_error("ReadCSV cannot open file " + filename);
  }

  std::string           buf;
  const char            delimiter = ',';
  std::string           field_value;
  fetch::math::SizeType row{0};
  fetch::math::SizeType col{0};

  // find number of rows and columns in the file
  while (std::getline(file, buf, '\n'))
  {
    if (row == 0)
    {
      std::stringstream ss(buf);
      while (std::getline(ss, field_value, delimiter))
      {
        ++col;
      }
    }
    ++row;
  }

  ArrayType weights({row - rows_to_skip, col - cols_to_skip});

  // read data into weights array
  std::string token;
  file.clear();
  file.seekg(0, std::ios::beg);

  while (rows_to_skip)
  {
    std::getline(file, buf, '\n');
    rows_to_skip--;
  }

  row = 0;
  while (std::getline(file, buf, '\n'))
  {
    col = 0;
    std::stringstream ss(buf);
    for (math::SizeType i = 0; i < cols_to_skip; i++)
    {
      std::getline(ss, field_value, delimiter);
    }
    while (std::getline(ss, field_value, delimiter))
    {
      weights(row, col) = static_cast<DataType>(stod(field_value));
      ++col;
    }
    ++row;
  }

  if (transpose)
  {
    weights = weights.Transpose();
  }
  return weights;
}

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
