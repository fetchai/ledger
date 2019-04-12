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

#include "core/logger.hpp"
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

  virtual ArrayType const &Evaluate()                                            = 0;
  virtual void             AddInput(std::shared_ptr<NodeInterface<T>> const &i)  = 0;
  virtual void             AddOutput(std::shared_ptr<NodeInterface<T>> const &i) = 0;
  virtual std::vector<std::pair<NodeInterface<T> *, ArrayType>> BackPropagate(
      ArrayType const &errorSignal)                                                = 0;
  virtual void ResetCache(bool input_size_changed)                                 = 0;
  virtual void SetBatch(bool b)                                                    = 0;
  virtual std::vector<std::shared_ptr<NodeInterface<T>>> const &GetOutputs() const = 0;
};

template <class T, class O>
class Node : public NodeInterface<T>, public O
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  template <typename... Params>
  Node(std::string const name, Params... params)
    : O(params...)
    , name_(std::move(name))
    , cached_output_present_(false)
    , batch_(false)
  {}

  virtual ~Node() = default;

  std::vector<std::reference_wrapper<const ArrayType>> GatherInputs() const
  {
    std::vector<std::reference_wrapper<const ArrayType>> inputs;
    for (auto const &i : inputs_)
    {
      inputs.push_back(i->Evaluate());
    }
    return inputs;
  }

  virtual ArrayType const &Evaluate()
  {
    std::vector<std::reference_wrapper<const ArrayType>> inputs = GatherInputs();
    FETCH_LOG_INFO("ML_LIB", "Evaluating node [", name_, "]");
    if (!cached_output_present_)
    {
      if (batch_)
      {
        cached_output_ = this->ForwardBatch(inputs);
      }
      else
      {
        cached_output_ = this->Forward(inputs, cached_output_);
      }
      cached_output_present_ = true;
    }

    return cached_output_;
  }

  virtual std::vector<std::pair<NodeInterface<T> *, ArrayType>> BackPropagate(
      ArrayType const &errorSignal)
  {
    FETCH_LOG_INFO("ML_LIB", "Backpropagating node [", name_, "]");
    std::vector<std::reference_wrapper<const ArrayType>> inputs = GatherInputs();
    std::vector<ArrayType> back_propagated_error_signals = this->Backward(inputs, errorSignal);
    std::vector<std::pair<NodeInterface<T> *, ArrayType>> non_back_propagated_error_signals;
    assert(back_propagated_error_signals.size() == inputs.size() || inputs.empty());

    for (std::uint64_t i(0); i < inputs_.size(); ++i)
    {
      auto ret = inputs_[i]->BackPropagate(back_propagated_error_signals[i]);
      non_back_propagated_error_signals.insert(non_back_propagated_error_signals.end(), ret.begin(),
                                               ret.end());
    }
    // If no input to backprop to, return gradient to caller
    // This is used to propagate outside of a SubGraph
    // The SubGraph has no knowledge of the rest of the network,
    // so it sends its unpropagated gradient to its wrapper node that will forward them out
    if (inputs_.empty())
    {
      for (auto g : back_propagated_error_signals)
      {
        non_back_propagated_error_signals.push_back(std::make_pair(this, g));
      }
    }
    return non_back_propagated_error_signals;
  }

  void AddInput(std::shared_ptr<NodeInterface<T>> const &i)
  {
    inputs_.push_back(i);
  }

  void AddOutput(std::shared_ptr<NodeInterface<T>> const &o)
  {
    outputs_.push_back(o);
  }

  virtual std::vector<std::shared_ptr<NodeInterface<T>>> const &GetOutputs() const
  {
    return outputs_;
  }

  virtual void ResetCache(bool input_size_changed)
  {
    cached_output_present_ = false;
    if (input_size_changed)
    {
      std::vector<std::reference_wrapper<const ArrayType>> inputs = GatherInputs();
      auto output_size = this->ComputeOutputShape(inputs);
      if (cached_output_.shape() != output_size)
      {
        cached_output_ = ArrayType(output_size);
      }
    }
  }

  virtual void SetBatch(bool b)
  {
    batch_ = b;
  }

private:
  std::vector<std::shared_ptr<NodeInterface<T>>> inputs_;
  std::vector<std::shared_ptr<NodeInterface<T>>> outputs_;
  std::string                                    name_;
  ArrayType                                      cached_output_;
  bool                                           cached_output_present_;
  bool                                           batch_;
};

}  // namespace ml
}  // namespace fetch
