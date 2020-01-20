#pragma once
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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace ml {

namespace ops {
template <class T>
class Ops;
}

struct OpsSaveableParams;

template <typename TensorType>
struct NodeSaveableParams;

enum class OpType : uint16_t;

template <typename TensorType>
class Node
{
private:
  enum class CachedOutputState : uint8_t
  {
    VALID_CACHE,
    CHANGED_CONTENT,
    CHANGED_SIZE
  };

public:
  using DataType        = typename TensorType::Type;
  using NodeWeakPtrType = std::weak_ptr<Node<TensorType>>;

  using VecTensorType    = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType           = fetch::ml::NodeSaveableParams<TensorType>;
  using NodeErrorMapType = std::unordered_map<Node<TensorType> *, std::vector<TensorType>>;

  ///////////////////////////////////
  /// CONSRTUCTORS / DESCTRUCTORS ///
  ///////////////////////////////////

  Node() = default;

  Node(OpType const &operation_type, std::string name,
       std::function<std::shared_ptr<ops::Ops<TensorType>>()> const &constructor)
    : name_(std::move(name))
    , cached_output_status_(CachedOutputState::CHANGED_SIZE)
    , operation_type_(operation_type)
  {
    op_ptr_ = constructor();
  }

  Node(OpType const &operation_type, std::string name, std::shared_ptr<ops::Ops<TensorType>> op_ptr)
    : name_(std::move(name))
    , cached_output_status_(CachedOutputState::CHANGED_SIZE)
    , operation_type_(operation_type)
    , op_ptr_(std::move(op_ptr))
  {}

  /**
   * This is used to make a copy of one node from another, i.e. when sharing weights.
   * @param old_node
   * @param name
   * @param op_ptr
   */
  Node(Node &old_node, std::string name, std::shared_ptr<ops::Ops<TensorType>> op_ptr)
    : name_(std::move(name))
    , cached_output_status_(CachedOutputState::CHANGED_SIZE)
    , operation_type_(old_node.get_op_type())
    , op_ptr_(std::move(op_ptr))
  {
    cached_output_ = old_node.cached_output_.Copy();
  }

  virtual ~Node() = default;

  ///////////////////////
  /// SAVEABLE PARAMS ///
  ///////////////////////

  std::shared_ptr<SPType> GetNodeSaveableParams() const;

  void SetNodeSaveableParams(NodeSaveableParams<TensorType> const &nsp,
                             std::shared_ptr<ops::Ops<TensorType>> op_ptr);

  ///////////////////////////////////
  /// FORWARD/BACKWARD OPERATIONS ///
  ///////////////////////////////////

  VecTensorType               GatherInputs() const;
  std::shared_ptr<TensorType> Evaluate(bool is_training);

  NodeErrorMapType BackPropagate(TensorType const &error_signal);

  void                                AddInput(NodeWeakPtrType const &i);
  std::vector<std::string>            GetInputNames();
  void                                AddOutput(NodeWeakPtrType const &o);
  std::vector<NodeWeakPtrType> const &GetOutputs() const;
  void                                ResetCache(bool input_size_changed);
  void                                ResetInputsAndOutputs();

  inline std::string const &GetNodeName()
  {
    return name_;
  }

  inline std::shared_ptr<ops::Ops<TensorType>> GetOp()
  {
    return op_ptr_;
  }

  /**
   * returns the stored operation type
   * @return
   */
  inline OpType const &get_op_type()
  {
    return operation_type_;
  }

  inline bool HasValidCache()
  {
    return static_cast<bool>(cached_output_status_ == CachedOutputState::VALID_CACHE);
  }

private:
  std::vector<NodeWeakPtrType> input_nodes_;
  std::vector<NodeWeakPtrType> outputs_;

  std::string       name_;
  TensorType        cached_output_;
  CachedOutputState cached_output_status_;
  OpType            operation_type_;

  std::shared_ptr<ops::Ops<TensorType>> op_ptr_;
};

}  // namespace ml
}  // namespace fetch
