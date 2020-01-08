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

#include "dmlf/shuffle_algorithm_interface.hpp"

namespace fetch {
namespace dmlf {

ShuffleAlgorithmInterface::ShuffleAlgorithmInterface(std::size_t count)
  : count_(count)
{}

std::size_t ShuffleAlgorithmInterface::GetCount() const
{
  // this impl is simple, but descendent ones may not be.
  return count_;
}

}  // namespace dmlf
}  // namespace fetch
