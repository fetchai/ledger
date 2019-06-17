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

#include "ml/optimisation/adam_optimiser.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include "ml/graph.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {

class VMAdamOptimiser : public fetch::vm::Object
{
public:
  using DataType      = float;
  using ArrayType     = fetch::math::Tensor<DataType>;
  using CriterionType = fetch::vm_modules::ml::VMCrossEntropyLoss;
  using GraphType     = fetch::ml::Graph<ArrayType>;

  VMAdamOptimiser(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                  std::shared_ptr<GraphType> const &graph,
                  std::vector<std::string> const &  input_node_names,
                  std::string const &               output_node_name)
    : fetch::vm::Object(vm, type_id)
    , optimiser_(graph, input_node_names, output_node_name)
  {}

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<fetch::vm_modules::ml::VMAdamOptimiser>("AdamOptimiser")
        .CreateConstuctor<fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph>,
                          fetch::vm::Ptr<fetch::vm::String>, fetch::vm::Ptr<fetch::vm::String>>()
        .CreateMemberFunction("Run", &fetch::vm_modules::ml::VMAdamOptimiser::Run);
  }

  static fetch::vm::Ptr<VMAdamOptimiser> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph> const &graph,
      fetch::vm::Ptr<fetch::vm::String> const &             input_node_names,
      fetch::vm::Ptr<fetch::vm::String> const &             output_node_names)
  {
    // std::make_shared<fetch::vm_modules::ml::VMGraph>(graph->graph_)
    return new VMAdamOptimiser(vm, type_id, graph, {input_node_names->str},
                               output_node_names->str);
  }

  DataType Run(fetch::vm::Ptr<fetch::vm_modules::ml::MnistDataLoader> const &loader)
  {
    return optimiser_.Run(loader->loader_);
  }

private:
  fetch::ml::optimisers::AdamOptimiser<ArrayType, CriterionType> optimiser_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
