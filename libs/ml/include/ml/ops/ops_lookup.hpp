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

#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/relu.hpp"

#include <typeindex>

namespace fetch {
namespace ml {
namespace ops {

// template <typename ArrayType>
// std::map<std::string, std::type_index> OpsMap = {{Dropout<ArrayType>::DESCRIPTOR,
// typeid(Dropout<ArrayType>)},
//                                                 {Relu<ArrayType>::DESCRIPTOR,
//                                                 typeid(Relu<ArrayType>)}};

template <typename ArrayType>
std::shared_ptr<fetch::ml::NodeInterface<ArrayType>> OpsLookup(SaveableParams<ArrayType> saved_node,
                                                               std::string               name)
{
  std::string                                          descrip = saved_node.DESCRIPTOR;
  std::shared_ptr<fetch::ml::NodeInterface<ArrayType>> node_ptr;

  if (descrip == ops::Dropout<ArrayType>::DESCRIPTOR)
  {
    auto castnode =
        dynamic_cast<SaveableParams<ArrayType>>(saved_node);  // todo: don't want to hardcode this
    node_ptr = std::make_shared<Node<ArrayType, ops::Dropout<ArrayType>>>(name, saved_node);
  }
  else if (descrip == ops::Relu<ArrayType>::DESCRIPTOR)
  {
    node_ptr = std::make_shared<Node<ArrayType, ops::Relu<ArrayType>>>(name, saved_node);
  }  // todo: other ops
  else
  {
    throw std::runtime_error("Unknown Op type");
  }
  return node_ptr;
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
