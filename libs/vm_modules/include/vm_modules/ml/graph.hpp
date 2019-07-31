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

#include "ml/graph.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMStateDict;

class VMGraph : public fetch::vm::Object
{
public:
  using DataType  = fetch::vm_modules::math::DataType;
  using GraphType = fetch::ml::Graph<fetch::math::Tensor<DataType>>;

  VMGraph(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMGraph> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  void SetInput(fetch::vm::Ptr<fetch::vm::String> const &                name,
                fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &input);

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> Evaluate(
      fetch::vm::Ptr<fetch::vm::String> const &name);

  void BackPropagateError(fetch::vm::Ptr<fetch::vm::String> const &name);

  void Step(DataType lr);

  void AddPlaceholder(fetch::vm::Ptr<fetch::vm::String> const &name);

  void AddFullyConnected(fetch::vm::Ptr<fetch::vm::String> const &name,
                         fetch::vm::Ptr<fetch::vm::String> const &input_name, int in, int out);

  void AddConv1D(fetch::vm::Ptr<fetch::vm::String> const &name,
                 fetch::vm::Ptr<fetch::vm::String> const &input_name, int filters, int in_channels,
                 int kernel_size, int stride_size);

  void AddRelu(fetch::vm::Ptr<fetch::vm::String> const &name,
               fetch::vm::Ptr<fetch::vm::String> const &input_name);

  void AddSoftmax(fetch::vm::Ptr<fetch::vm::String> const &name,
                  fetch::vm::Ptr<fetch::vm::String> const &input_name);

  void AddCrossEntropyLoss(fetch::vm::Ptr<fetch::vm::String> const &name,
                           fetch::vm::Ptr<fetch::vm::String> const &input_name,
                           fetch::vm::Ptr<fetch::vm::String> const &label_name);

  void AddMeanSquareErrorLoss(fetch::vm::Ptr<fetch::vm::String> const &name,
                              fetch::vm::Ptr<fetch::vm::String> const &input_name,
                              fetch::vm::Ptr<fetch::vm::String> const &label_name);

  void AddDropout(fetch::vm::Ptr<fetch::vm::String> const &name,
                  fetch::vm::Ptr<fetch::vm::String> const &input_name, DataType const &prob);

  void LoadStateDict(fetch::vm::Ptr<VMStateDict> const &sd);

  fetch::vm::Ptr<VMStateDict> StateDict();

  static void Bind(fetch::vm::Module &module);

  GraphType graph_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
