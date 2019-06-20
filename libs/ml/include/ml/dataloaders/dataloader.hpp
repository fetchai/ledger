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

template <typename LabelType, typename DataType>
class DataLoader
{
public:
  DataLoader(bool random_mode)
    : random_mode_(random_mode)
  {}

  using SizeType = fetch::math::SizeType;

  virtual ~DataLoader()                                         = default;
  virtual std::pair<LabelType, std::vector<DataType>> GetNext() = 0;
  virtual std::pair<LabelType, std::vector<DataType>> SubsetToArray(
      fetch::math::SizeType subset_size);
  virtual std::uint64_t Size() const   = 0;
  virtual bool          IsDone() const = 0;
  virtual void          Reset()        = 0;

protected:
  bool random_mode_ = false;
};

template <typename LabelType, typename DataType>
std::pair<LabelType, std::vector<DataType>> DataLoader<LabelType, DataType>::SubsetToArray(
    fetch::math::SizeType subset_size)
{
  if (IsDone())
  {
    Reset();
  }
  std::pair<LabelType, std::vector<DataType>> training_pair = GetNext();
  std::vector<SizeType>                       labels_size   = training_pair.first.shape();
  std::vector<DataType>                       data;

  // Prepare output data tensors
  for (SizeType i{0}; i < training_pair.second.size(); i++)
  {
    std::vector<SizeType> current_data_shape             = training_pair.second.at(i).shape();
    current_data_shape.at(current_data_shape.size() - 1) = subset_size;
    data.push_back(DataType{current_data_shape});
  }

  // Prepare output label tensor
  SizeType label_batch_dimension        = labels_size.size() - 1;
  labels_size.at(label_batch_dimension) = subset_size;
  LabelType labels{labels_size};

  SizeType i{0};
  do
  {
    // Fill label slice
    auto label_slice = labels.Slice(i, label_batch_dimension);
    label_slice.Assign(training_pair.first);

    // Fill all data from data vector
    for (SizeType j{0}; j < training_pair.second.size(); j++)
    {
      // Fill data[j] slice
      auto data_slice = data.at(j).Slice(i, training_pair.second.at(j).shape().size() - 1);
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

  return std::make_pair(labels, data);
}

}  // namespace ml
}  // namespace fetch
