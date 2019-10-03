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
#include <vector>

namespace fetch {
namespace dmlf {

class ShuffleAlgorithmInterface
{
public:
  explicit ShuffleAlgorithmInterface(std::size_t count);

  virtual ~ShuffleAlgorithmInterface() = default;

  virtual std::vector<std::size_t> GetNextOutputs() = 0;

  std::size_t GetCount() const;

  ShuffleAlgorithmInterface(const ShuffleAlgorithmInterface &other) = delete;
  ShuffleAlgorithmInterface &operator=(const ShuffleAlgorithmInterface &other)  = delete;
  bool                       operator==(const ShuffleAlgorithmInterface &other) = delete;
  bool                       operator<(const ShuffleAlgorithmInterface &other)  = delete;

protected:
private:
  std::size_t count_;
};

}  // namespace dmlf
}  // namespace fetch
