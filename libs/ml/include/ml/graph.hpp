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

#include <iostream>
#include <memory>
#include <unordered_map>

#include "ml/node.hpp"
#include "ml/ops/placeholder.hpp"
#include "core/logger.hpp"  // for FETCH_LOG_INFO

namespace fetch {
namespace ml {

template <class T>
class Graph
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Graph()
  {}

  ArrayPtrType Evaluate(std::string const &nodeName)
  {
    return nodes_[nodeName]->Evaluate();
  }

  template <class OperationType, typename... Params>
  void AddNode(std::string const &nodeName, std::vector<std::string> const &inputs,
               Params... params)
  {
    nodes_[nodeName] = std::make_shared<Node<ArrayType, OperationType>>(nodeName, params...);
    FETCH_LOG_INFO("ML_LIB", "Creating node [", nodeName, "]");
    for (auto const &i : inputs)
    {
      nodes_[nodeName]->AddInput(nodes_[i]);
    }
  }

  void SetInput(std::string const &nodeName, ArrayPtrType data)
  {
    std::shared_ptr<fetch::ml::ops::PlaceHolder<ArrayType>> placeholder =
        std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<ArrayType>>(nodes_[nodeName]);
    placeholder->SetData(data);
  }

protected:
  std::unordered_map<std::string, std::shared_ptr<fetch::ml::NodeInterface<ArrayType>>> nodes_;
};

}  // namespace ml
}  // namespace fetch
