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
#include "ml/optimisation/adagrad_optimiser.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/momentum_optimiser.hpp"
#include "ml/optimisation/optimiser.hpp"
#include "ml/optimisation/rmsprop_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/optimisation/optimiser.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

using ArrayType = fetch::math::Tensor<VMOptimiser::DataType>;
using GraphType = fetch::ml::Graph<ArrayType>;

using OptimiserType         = fetch::ml::optimisers::Optimiser<ArrayType>;
using AdagradOptimiserType  = fetch::ml::optimisers::AdaGradOptimiser<ArrayType>;
using AdamOptimiserType     = fetch::ml::optimisers::AdamOptimiser<ArrayType>;
using MomentumOptimiserType = fetch::ml::optimisers::MomentumOptimiser<ArrayType>;
using RmspropOptimiserType  = fetch::ml::optimisers::RMSPropOptimiser<ArrayType>;
using SgdOptimiserType      = fetch::ml::optimisers::SGDOptimiser<ArrayType>;

VMOptimiser::VMOptimiser(VM *vm, TypeId type_id, std::string const &mode, GraphType const &graph,
                         std::vector<std::string> const &input_node_names,
                         std::string const &label_node_name, std::string const &output_node_name)
  : Object(vm, type_id)
{
  if (mode == "adagrad")
  {
    AdagradOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                   label_node_name, output_node_name);
    optimiser_ = std::make_shared<AdagradOptimiserType>(optimiser);
  }
  else if (mode == "adam")
  {
    AdamOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                label_node_name, output_node_name);
    optimiser_ = std::make_shared<AdamOptimiserType>(optimiser);
  }
  else if (mode == "momentum")
  {
    MomentumOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                    label_node_name, output_node_name);
    optimiser_ = std::make_shared<MomentumOptimiserType>(optimiser);
  }
  else if (mode == "rmsprop")
  {
    RmspropOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                   label_node_name, output_node_name);
    optimiser_ = std::make_shared<RmspropOptimiserType>(optimiser);
  }
  else if (mode == "sgd")
  {
    SgdOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                               label_node_name, output_node_name);
    optimiser_ = std::make_shared<SgdOptimiserType>(optimiser);
  }
  else
  {
    throw std::runtime_error("unrecognised optimiser mode");
  }
}

void VMOptimiser::Bind(Module &module)
{
  module.CreateClassType<VMOptimiser>("Optimiser")
      .CreateConstructor(&VMOptimiser::Constructor)
      .CreateMemberFunction("run", &VMOptimiser::RunData)
      .CreateMemberFunction("run", &VMOptimiser::RunLoader)
      .CreateMemberFunction("run", &VMOptimiser::RunLoaderNoSubset);
}

Ptr<VMOptimiser> VMOptimiser::Constructor(VM *vm, TypeId type_id, Ptr<String> const &mode,
                                          Ptr<VMGraph> const &graph,
                                          Ptr<String> const & input_node_names,
                                          Ptr<String> const & label_node_name,
                                          Ptr<String> const & output_node_names)
{
  return new VMOptimiser(vm, type_id, mode->str, graph->graph_, {input_node_names->str},
                         label_node_name->str, output_node_names->str);
}

VMOptimiser::DataType VMOptimiser::RunData(Ptr<fetch::vm_modules::math::VMTensor> const &data,
                                           Ptr<fetch::vm_modules::math::VMTensor> const &labels,
                                           uint64_t                                      batch_size)
{
  return optimiser_->Run({(data->GetTensor())}, labels->GetTensor(), batch_size);
}

VMOptimiser::DataType VMOptimiser::RunLoader(Ptr<VMDataLoader> const &loader, uint64_t batch_size,
                                             uint64_t subset_size)
{
  return optimiser_->Run(*(loader->loader_), batch_size, subset_size);
}

VMOptimiser::DataType VMOptimiser::RunLoaderNoSubset(Ptr<VMDataLoader> const &loader,
                                                     uint64_t                 batch_size)
{
  return optimiser_->Run(*(loader->loader_), batch_size);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
