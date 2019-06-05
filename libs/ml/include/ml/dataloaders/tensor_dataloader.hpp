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

#include "ml/dataloaders/dataloader.hpp"

#include <vector>

namespace fetch {
namespace ml {

template <typename LabelType, typename DataType>
class TensorDataLoader : DataLoader<LabelType, DataType>
{
public:
  virtual std::pair<LabelType, std::vector<DataType>> GetNext()
  {
    return data_.at(cursor_++);
  }

  virtual uint64_t Size() const
  {
    return data_.size();
  }

  virtual bool IsDone() const
  {
    return (data_.empty() || cursor_ >= data_.size());
  }

  virtual void Reset()
  {
    cursor_ = 0;
  }

  void Add(std::pair<LabelType, std::vector<DataType>> const &data)
  {
    data_.push_back(data);
  }

private:
  uint64_t                                                 cursor_ = 0;
  std::vector<std::pair<LabelType, std::vector<DataType>>> data_;
};

}  // namespace ml
}  // namespace fetch
