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

#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace fetch {

namespace math {
template <typename T, typename C>
class Tensor;
}
namespace ml {
template <class T>
class Graph;
namespace optimisers {
template <class T>
class Optimiser;
}
}  // namespace ml
namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMOptimiser : public fetch::vm::Object
{
public:
  using DataType = fetch::vm_modules::math::DataType;

  VMOptimiser(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &mode,
              fetch::ml::Graph<fetch::math::Tensor<DataType>> const &graph,
              std::vector<std::string> const &input_node_names, std::string const &label_node_name,
              std::string const &output_node_name);

  static void Bind(vm::Module &module);

  static fetch::vm::Ptr<VMOptimiser> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &mode,
      fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph> const &graph,
      fetch::vm::Ptr<fetch::vm::String> const &             input_node_names,
      fetch::vm::Ptr<fetch::vm::String> const &             label_node_name,
      fetch::vm::Ptr<fetch::vm::String> const &             output_node_names);

  DataType RunData(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &data,
                   fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels,
                   uint64_t                                                 batch_size);

  DataType RunLoader(fetch::vm::Ptr<fetch::vm_modules::ml::VMDataLoader> const &loader,
                     uint64_t batch_size, uint64_t subset_size);

  DataType RunLoaderNoSubset(fetch::vm::Ptr<fetch::vm_modules::ml::VMDataLoader> const &loader,
                             uint64_t                                                   batch_size);

private:
  std::shared_ptr<fetch::ml::optimisers::Optimiser<fetch::math::Tensor<DataType>>> optimiser_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
