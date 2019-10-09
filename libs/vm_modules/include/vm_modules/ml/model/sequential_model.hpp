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

#include "ml/model/model.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMStateDict;

class VMSequentialModel : public fetch::vm::Object
{
public:
  using DataType  = fetch::vm_modules::math::DataType;
  using ModelType = fetch::ml::model::Model;
  using GraphType = fetch::ml::Graph<fetch::math::Tensor<DataType>>;

  VMSequentialModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMSequentialModel> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  void Add(fetch::vm::Ptr<fetch::vm::String> const &layer, fetch::vm::Ptr<fetch::vm::String> const &hidden_nodes);

  void Compile(fetch::vm::Ptr<fetch::vm::String> const &loss, fetch::vm::Ptr<fetch::vm::String> const &optimiser);

  void Fit(fetch::vm::Ptr<VMTensor> const &data, fetch::vm::Ptr<VMTensor> const &labels, fetch::math::SizeType batch_size);

  void Evaluate();

  static void Bind(fetch::vm::Module &module);

private:
  TensorDataLoader dl_;
  ModelType model_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
