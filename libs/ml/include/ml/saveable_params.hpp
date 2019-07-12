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

#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {

/* Generic container for all the saveable params of an op.
 * Some ops will have to declare subclasses (substructs?) of this.
 */
template <class T>
struct SaveableParams
{
  std::string DESCRIPTOR;  // description of op
};

template <class T>
struct TrainableSaveableParams : SaveableParams<T>
{
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  ArrayPtrType weights_;
};

template <class T>
struct GraphSaveableParams
{
  std::vector<std::pair<std::string, std::vector<std::string>>>
                                                     connections;  // unique node name to list of inputs
  std::unordered_map<std::string, SaveableParams<T>> nodes;
};
}  // namespace ml
}  // namespace fetch
