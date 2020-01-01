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

#include "ml/optimisation/optimiser.hpp"

#include "ml/optimisation/adagrad_optimiser.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/momentum_optimiser.hpp"
#include "ml/optimisation/rmsprop_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/serializers/ml_types.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/optimisation/optimiser.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

VMOptimiser::VMOptimiser(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{
  mode_ = OptimiserMode::NONE;
}

VMOptimiser::VMOptimiser(VM *vm, TypeId type_id, std::string const &mode, GraphType const &graph,
                         Ptr<VMDataLoader> const &       loader,
                         std::vector<std::string> const &input_node_names,
                         std::string const &label_node_name, std::string const &output_node_name)
  : Object(vm, type_id)
{
  if (mode == "adagrad")
  {
    mode_ = OptimiserMode::ADAGRAD;
    AdagradOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                   label_node_name, output_node_name);
    optimiser_ = std::make_shared<AdagradOptimiserType>(optimiser);
  }
  else if (mode == "adam")
  {
    mode_ = OptimiserMode::ADAM;
    AdamOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                label_node_name, output_node_name);
    optimiser_ = std::make_shared<AdamOptimiserType>(optimiser);
  }
  else if (mode == "momentum")
  {
    mode_ = OptimiserMode::MOMENTUM;
    MomentumOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                    label_node_name, output_node_name);
    optimiser_ = std::make_shared<MomentumOptimiserType>(optimiser);
  }
  else if (mode == "rmsprop")
  {
    mode_ = OptimiserMode::RMSPROP;
    RmspropOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                                   label_node_name, output_node_name);
    optimiser_ = std::make_shared<RmspropOptimiserType>(optimiser);
  }
  else if (mode == "sgd")
  {
    mode_ = OptimiserMode::SGD;
    SgdOptimiserType optimiser(std::make_shared<GraphType>(graph), input_node_names,
                               label_node_name, output_node_name);
    optimiser_ = std::make_shared<SgdOptimiserType>(optimiser);
  }
  else
  {
    RuntimeError("unrecognised optimiser mode: " + mode);
    return;
  }
  loader_ = (loader->GetDataLoader());
}

void VMOptimiser::Bind(Module &module, bool const enable_experimental)
{
  if (enable_experimental)
  {
    module.CreateClassType<VMOptimiser>("Optimiser")
        .CreateConstructor(&VMOptimiser::Constructor, vm::MAXIMUM_CHARGE)
        .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMOptimiser> {
          return Ptr<VMOptimiser>{new VMOptimiser(vm, type_id)};
        })
        .CreateMemberFunction("run", &VMOptimiser::RunData, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("run", &VMOptimiser::RunLoader, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("run", &VMOptimiser::RunLoaderNoSubset, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("setGraph", &VMOptimiser::SetGraph, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("setDataloader", &VMOptimiser::SetDataloader, vm::MAXIMUM_CHARGE);
  }
}

Ptr<VMOptimiser> VMOptimiser::Constructor(
    VM *vm, TypeId type_id, Ptr<String> const &mode, Ptr<VMGraph> const &graph,
    Ptr<VMDataLoader> const &                            loader,
    Ptr<fetch::vm::Array<Ptr<fetch::vm::String>>> const &input_node_names,
    Ptr<String> const &label_node_name, Ptr<String> const &output_node_names)
{
  auto                     n_elements = input_node_names->elements.size();
  std::vector<std::string> input_names(n_elements);

  for (fetch::math::SizeType i{0}; i < n_elements; i++)
  {
    Ptr<fetch::vm::String> ptr_string = input_node_names->elements.at(i);
    input_names.at(i)                 = (ptr_string)->string();
  }

  return Ptr<VMOptimiser>{new VMOptimiser(vm, type_id, mode->string(), graph->GetGraph(), loader,
                                          input_names, label_node_name->string(),
                                          output_node_names->string())};
}

VMOptimiser::DataType VMOptimiser::RunData(Ptr<fetch::vm_modules::math::VMTensor> const &data,
                                           Ptr<fetch::vm_modules::math::VMTensor> const &labels,
                                           uint64_t                                      batch_size)
{
  return optimiser_->Run({(data->GetTensor())}, labels->GetTensor(), batch_size);
}

VMOptimiser::DataType VMOptimiser::RunLoader(uint64_t batch_size, uint64_t subset_size)
{
  return optimiser_->Run(*loader_, batch_size, subset_size);
}

VMOptimiser::DataType VMOptimiser::RunLoaderNoSubset(uint64_t batch_size)
{
  return optimiser_->Run(*loader_, batch_size);
}

void VMOptimiser::SetGraph(Ptr<VMGraph> const &graph)
{
  *(optimiser_->GetGraph()) = graph->GetGraph();
}

void VMOptimiser::SetDataloader(Ptr<VMDataLoader> const &loader)
{
  *loader_ = *(loader->GetDataLoader());
}

bool VMOptimiser::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << *this;
  return true;
}

bool VMOptimiser::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer.seek(0);
  auto opt = std::make_shared<VMOptimiser>(this->vm_, this->type_id_);
  buffer >> *opt;
  *this = *opt;
  return true;
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
