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

#include <memory>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename LabelType, typename DataType>
class DataLoader
{
  using SizeType = fetch::math::SizeType;

public:
  explicit DataLoader(bool random_mode)
    : random_mode_(random_mode)
  {}
  virtual ~DataLoader() = default;

  virtual std::pair<LabelType, std::vector<DataType>> GetNext() = 0;

  virtual SizeType Size() const   = 0;
  virtual bool     IsDone() const = 0;
  virtual void     Reset()        = 0;

protected:
  bool random_mode_ = false;
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
