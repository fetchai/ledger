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

#include "ops/ops.hpp"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ml {

template <class T>
class NodeInterface
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  virtual ArrayPtrType Evaluate() = 0;
  virtual void AddInput(std::shared_ptr<NodeInterface<T>> const &i) = 0;
};

template <class T, class O>
class Node : public NodeInterface<T>, public O
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Node(std::string const name)
    : name_(std::move(name))
  {}

  virtual ~Node() = default;

  ArrayPtrType Evaluate()
  {
    std::vector<ArrayPtrType> inputs;
    for (auto const &i : inputs_)
    {
      inputs.push_back(i->Evaluate());
    }
    std::cout << "Evaluating node [" << name_ << "]" << std::endl;
    return this->Forward(inputs);
  }

  void AddInput(std::shared_ptr<NodeInterface<T>> const &i)
  {
    inputs_.push_back(i);
  }

private:
  std::vector<std::shared_ptr<NodeInterface<T>>> inputs_;
  std::string                                    name_;
};

}  // namespace ml
}  // namespace fetch
