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

#include "math/tensor/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gmock/gmock.h"

#include <sstream>

#include "benchmark/benchmark.h"

#include <vector>

namespace vm_modules {
namespace benchmark {
namespace ml {
namespace model {

namespace {

using VMPtr        = std::shared_ptr<fetch::vm::VM>;
using SizeType     = fetch::math::SizeType;
using SizeRef      = fetch::math::SizeType const &;
using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

//////////////////////////
// VM Objects factories //
//////////////////////////

VMPtr NewVM()
{
  using VMFactory = fetch::vm_modules::VMFactory;

  // setup the VM
  auto module = VMFactory::GetModule(fetch::vm_modules::VMFactory::USE_ALL);
  auto vm     = std::make_shared<fetch::vm::VM>(module.get());

  return vm;
}

fetch::vm::Ptr<fetch::vm::String> vmString(VMPtr &vm, std::string const &str)
{
  return fetch::vm::Ptr<fetch::vm::String>{new fetch::vm::String{vm.get(), str}};
}

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vmTensor(VMPtr &                      vm,
                                                           std::vector<SizeType> const &shape)
{
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(shape);
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(VMPtr &vm)
{
  auto model_category = vmString(vm, "sequential");
  auto model          = vm->CreateNewObject<fetch::vm_modules::ml::model::VMModel>(model_category);
  return model;
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations)
{
  if (sizes.size() != (activations.size() + 1))
  {
    throw std::runtime_error{"Wrong configuration for multilayer VMModel"};
  }

  auto model           = vmSequentialModel(vm);
  auto size            = activations.size();
  auto layer_type      = vmString(vm, "dense");
  auto activation_type = vmString(vm, "relu");

  for (std::size_t i{0}; i < size; ++i)
  {
    auto input_size  = sizes[i];
    auto output_size = sizes[i + 1];

    if (activations[i])
    {
      model->Estimator().LayerAddDenseActivation(layer_type, input_size, output_size,
                                                 activation_type);
      model->LayerAddDenseActivation(layer_type, input_size, output_size, activation_type);
    }
    else
    {
      model->Estimator().LayerAddDense(layer_type, input_size, output_size);
      model->LayerAddDense(layer_type, input_size, output_size);
    }
  }

  return model;
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations,
    std::string const &loss, std::string const &optimiser)
{
  auto model = vmSequentialModel(vm, sizes, activations);

  // compile model
  auto vm_loss      = vmString(vm, loss);
  auto vm_optimiser = vmString(vm, optimiser);
  model->Estimator().CompileSequential(vm_loss, vm_optimiser);
  model->CompileSequential(vm_loss, vm_optimiser);

  return model;
}

////////////////
// Benchmarks //
////////////////

///*

struct BM_AddLayer_config
{
  explicit BM_AddLayer_config(::benchmark::State const &state)
  {
    input_size  = static_cast<SizeType>(state.range(0));
    output_size = static_cast<SizeType>(state.range(1));
    activation  = static_cast<bool>(state.range(2));
  }

  SizeType input_size;
  SizeType output_size;
  bool     activation;
};

void BM_AddLayer(::benchmark::State &state)
{
  for (auto _ : state)
  {
    state.PauseTiming();

    // Get config
    BM_AddLayer_config config{state};

    // model
    auto vm    = NewVM();
    auto model = vmSequentialModel(vm);
    // arguments list
    auto layer_type      = vmString(vm, "dense");
    auto activation_type = vmString(vm, "relu");

    if (config.activation)
    {
      state.counters["charge"] = static_cast<double>(
          model->Estimator().LayerAddDense(layer_type, config.input_size, config.output_size));

      state.counters["PaddedSizesSum"] =
          static_cast<double>(model->Estimator().GetPaddedSizesSum());
      state.counters["SizesSum"] = static_cast<double>(model->Estimator().GetSizesSum());

      state.ResumeTiming();
      model->LayerAddDense(layer_type, config.input_size, config.output_size);
    }
    else
    {
      state.counters["charge"] = static_cast<double>(model->Estimator().LayerAddDenseActivation(
          layer_type, config.input_size, config.output_size, activation_type));

      state.counters["PaddedSizesSum"] =
          static_cast<double>(model->Estimator().GetPaddedSizesSum());
      state.counters["SizesSum"] = static_cast<double>(model->Estimator().GetSizesSum());

      state.ResumeTiming();
      model->LayerAddDenseActivation(layer_type, config.input_size, config.output_size,
                                     activation_type);
    }
  }
}

// (BM_AddLayer_config) input_size, output_size, activation
BENCHMARK(BM_AddLayer)->Args({1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1000, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1000, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1, 10000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1, 100000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({200, 200, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({2000, 20, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({3000, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 3000, 0})->Unit(::benchmark::kMicrosecond);
//*/

///*
struct BM_Predict_config
{
  explicit BM_Predict_config(::benchmark::State const &state)
  {
    batch_size    = static_cast<SizeType>(state.range(0));
    layers_number = static_cast<SizeType>(state.range(1));

    sizes.reserve(layers_number);
    for (std::size_t i = 0; i < layers_number; ++i)
    {
      sizes.emplace_back(static_cast<SizeType>(state.range(2 + i)));
    }
    activations.reserve(layers_number - 1);
    for (std::size_t i = 0; i < (layers_number - 1); ++i)
    {
      activations.emplace_back(static_cast<bool>(state.range(2 + layers_number + i)));
    }
  }

  SizeType              batch_size;
  SizeType              layers_number;
  std::vector<SizeType> sizes;        // layers input/output sizes
  std::vector<bool>     activations;  // layers activations
};

void BM_Predict(::benchmark::State &state)
{
  // Get args form state
  BM_Predict_config config{state};

  for (auto _ : state)
  {
    state.PauseTiming();
    // set up a compiled model
    auto vm    = NewVM();
    auto model = vmSequentialModel(vm, config.sizes, config.activations, "mse", "adam");

    // predict
    std::vector<SizeType> data_shape{config.sizes[0], config.batch_size};
    auto                  data    = vmTensor(vm, data_shape);
    state.counters["charge"]      = static_cast<double>(model->Estimator().Predict(data));
    state.counters["ForwardCost"] = static_cast<double>(model->Estimator().GetForwardCost());
    state.counters["OpsCount"]    = static_cast<double>(model->Estimator().GetOpsCount());

    state.ResumeTiming();
    auto res = model->Predict(data);
  }
}

// (BM_Predict_config) batch_size, number_of_layers, input_size, hidden_1_size, ...., output_size,
// activation_3,....

// BOSTON
BENCHMARK(BM_Predict)->Args({1, 4, 13, 10, 10, 1, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({10, 4, 13, 10, 10, 1, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({32, 4, 13, 10, 10, 1, 1, 1, 0})->Unit(::benchmark::kMicrosecond);

// MNIST
BENCHMARK(BM_Predict)->Args({1, 3, 784, 10, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({10, 3, 784, 10, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({32, 3, 784, 10, 10, 1, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({1, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({2, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({4, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({8, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({16, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({32, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({64, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({1, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({2, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({4, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({8, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({16, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({32, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({64, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({128, 4, 1, 1, 1, 1, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({256, 4, 1, 1, 1, 1, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({512, 4, 1, 1, 1, 1, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1024, 4, 1, 1, 1, 1, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({2048, 4, 1, 1, 1, 1, 0, 0, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({128, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({1024, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({2048, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({128, 5, 10000, 1, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 10000, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 1, 10000, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 1, 1, 10000, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 1, 1, 1, 10000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({512, 5, 10000, 1, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 10000, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 1, 10000, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 1, 1, 10000, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 1, 1, 1, 10000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 2, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 10000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 100000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 3, 1, 1, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 10, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 100, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 10000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 100000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 2, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 100, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 10000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 100000, 1, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 2, 10000, 10000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 100, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({128, 5, 1000, 1000, 1000, 1000, 1000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 5, 1000, 1000, 1000, 1000, 1000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1000, 1000, 1000, 1000, 1000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
//*/

///*
struct BM_Compile_config
{
  explicit BM_Compile_config(::benchmark::State const &state)
  {
    layers_number = static_cast<SizeType>(state.range(0));

    sizes.reserve(layers_number);
    for (std::size_t i = 0; i < layers_number; ++i)
    {
      sizes.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }
    activations.reserve(layers_number - 1);
    for (std::size_t i = 0; i < (layers_number - 1); ++i)
    {
      activations.emplace_back(static_cast<bool>(state.range(1 + layers_number + i)));
    }
  }

  SizeType              layers_number;
  std::vector<SizeType> sizes;        // layers input/output sizes
  std::vector<bool>     activations;  // layers activations
};

void BM_Compile(::benchmark::State &state)
{

  for (auto _ : state)
  {
    state.PauseTiming();

    // Get args form state
    BM_Compile_config config{state};

    // set up model
    auto vm    = NewVM();
    auto model = vmSequentialModel(vm, config.sizes, config.activations);

    // compile model
    auto loss      = vmString(vm, "mse");
    auto optimiser = vmString(vm, "adam");
    state.counters["charge"] =
        static_cast<double>(model->Estimator().CompileSequential(loss, optimiser));

    state.counters["PaddedSizesSum"] = static_cast<double>(model->Estimator().GetPaddedSizesSum());
    state.counters["SizesSum"]       = static_cast<double>(model->Estimator().GetSizesSum());

    state.ResumeTiming();
    model->CompileSequential(loss, optimiser);
  }
}

// (BM_Compile_config) number_of_layers, input_size, hidden_1_size, ...., output_size,
// activation_1,....
BENCHMARK(BM_Compile)->Args({2, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 10000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 100000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 1000000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 10000000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({2, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 100, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 10000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 100000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1000000, 1, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({2, 10000, 10000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 100, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)
    ->Args({6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)
    ->Args({5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({4, 1, 1, 1, 1, 0, 0, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({8, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({5, 10000, 1, 1, 1, 1, 0, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({5, 1, 10000, 1, 1, 1, 0, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({5, 1, 1, 10000, 1, 1, 0, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({5, 1, 1, 1, 10000, 1, 0, 0, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({5, 1, 1, 1, 1, 10000, 0, 0, 0, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({3, 1, 1, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 10, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 100, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 10000, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 100000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)
    ->Args({5, 1000, 1000, 1000, 1000, 1000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
//*/

///*
struct BM_Fit_config
{
  explicit BM_Fit_config(::benchmark::State const &state)
  {
    datapoints_number = static_cast<SizeType>(state.range(0));
    batch_size        = static_cast<SizeType>(state.range(1));
    layers_number     = static_cast<SizeType>(state.range(2));

    sizes.reserve(layers_number);
    for (std::size_t i = 0; i < layers_number; ++i)
    {
      sizes.emplace_back(static_cast<SizeType>(state.range(3 + i)));
    }
    activations.reserve(layers_number - 1);
    for (std::size_t i = 0; i < (layers_number - 1); ++i)
    {
      activations.emplace_back(static_cast<bool>(state.range(3 + layers_number + i)));
    }
  }

  SizeType              datapoints_number;
  SizeType              batch_size;
  SizeType              layers_number;
  std::vector<SizeType> sizes;        // layers input/output sizes
  std::vector<bool>     activations;  // layers activations
};

void BM_Fit(::benchmark::State &state)
{
  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  // Get args form state
  BM_Fit_config config{state};

  for (auto _ : state)
  {
    state.PauseTiming();

    // set up a compiled model
    auto vm    = NewVM();
    auto model = vmSequentialModel(vm, config.sizes, config.activations, "mse", "adam");

    // set up data and labels
    std::vector<uint64_t> data_shape{config.sizes[0], config.datapoints_number};
    std::vector<uint64_t> label_shape{config.sizes[config.sizes.size() - 1],
                                      config.datapoints_number};
    auto                  data  = vmTensor(vm, data_shape);
    auto                  label = vmTensor(vm, label_shape);

    // fit
    state.counters["charge"] =
        static_cast<double>(model->Estimator().Fit(data, label, config.batch_size));

    state.counters["PaddedSizesSum"] = static_cast<double>(model->Estimator().GetPaddedSizesSum());
    state.counters["SizesSum"]       = static_cast<double>(model->Estimator().GetSizesSum());

    state.ResumeTiming();
    model->Fit(data, label, config.batch_size);
  }
}

// (BM_Fit_config) n_datapoints, batch_size, num_layers, in_size, hidden_1_size, ...., out_size,
// activation_1,....

// BOSTON
BENCHMARK(BM_Fit)->Args({1, 1, 4, 13, 10, 10, 1, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 10, 4, 13, 10, 10, 1, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({32, 32, 4, 13, 10, 10, 1, 1, 1, 0})->Unit(::benchmark::kMicrosecond);

// MNIST
BENCHMARK(BM_Fit)->Args({1, 1, 3, 784, 10, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({320, 32, 3, 784, 100, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({3200, 32, 3, 784, 100, 10, 1, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1000, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10000, 2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1000, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10000, 2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1000, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10000, 2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1000, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10000, 3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)
    ->Args({10, 1, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({100, 1, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({1000, 1, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 1, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 10, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 100, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 1000, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 10000, 5, 10, 100, 1, 100, 10, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({1, 1, 3, 1, 1000000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1, 1, 2, 1000000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1, 1, 2, 1, 1000000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1, 1, 2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 3, 1, 1000000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 1, 2, 1000000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 1, 2, 1, 1000000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 1, 2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 10, 3, 1, 1000000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 10, 2, 1000000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 10, 2, 1, 1000000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 10, 2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({100, 10, 3, 1, 1000000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 10, 2, 1000000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 10, 2, 1, 1000000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 10, 2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)
    ->Args({1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10, 10, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({100, 10, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({100, 100, 8, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
//*/

///*
struct BM_SerializeToString_config
{
  explicit BM_SerializeToString_config(::benchmark::State const &state)
  {
    layers_number = static_cast<SizeType>(state.range(0));

    sizes.reserve(layers_number);
    for (std::size_t i = 0; i < layers_number; ++i)
    {
      sizes.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }
    activations.reserve(layers_number - 1);
    for (std::size_t i = 0; i < (layers_number - 1); ++i)
    {
      activations.emplace_back(static_cast<bool>(state.range(1 + layers_number + i)));
    }
  }

  SizeType              layers_number;
  std::vector<SizeType> sizes;        // layers input/output sizes
  std::vector<bool>     activations;  // layers activations
};

void BM_SerializeToString(::benchmark::State &state)
{
  // Get args form state
  BM_SerializeToString_config config{state};

  for (auto _ : state)
  {
    state.PauseTiming();
    // set up a compiled model
    auto vm    = NewVM();
    auto model = vmSequentialModel(vm, config.sizes, config.activations, "mse", "adam");

    // serialise to string
    state.counters["charge"] = static_cast<double>(model->Estimator().SerializeToString());

    state.counters["OpsCount"]       = static_cast<double>(model->Estimator().GetOpsCount());
    state.counters["PaddedSizesSum"] = static_cast<double>(model->Estimator().GetPaddedSizesSum());
    state.counters["SizesSum"]       = static_cast<double>(model->Estimator().GetSizesSum());

    state.ResumeTiming();

    model->SerializeToString();
  }
}

//// (BM_SerializeToString_config) number_of_layers, input_size, hidden_1_size, ...., output_size,
/// hidden_1_activation, ...
BENCHMARK(BM_SerializeToString)->Args({2, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1, 10000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1, 100000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1, 1000000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1, 10000000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SerializeToString)->Args({2, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 100, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 10000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 100000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 1000000, 1, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SerializeToString)->Args({2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 100, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SerializeToString)
    ->Args({6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SerializeToString)
    ->Args({5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({4, 1, 1, 1, 1, 0, 0, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SerializeToString)->Args({8, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SerializeToString)
    ->Args({5, 100000, 1, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)
    ->Args({5, 1, 100000, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)
    ->Args({5, 1, 1, 100000, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)
    ->Args({5, 1, 1, 1, 100000, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)
    ->Args({5, 1, 1, 1, 1, 100000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SerializeToString)->Args({3, 1, 1, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({3, 1, 10, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({3, 1, 100, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({3, 1, 10000, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({3, 1, 100000, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SerializeToString)->Args({3, 1, 100000, 0, 0})->Unit(::benchmark::kMicrosecond);

void BM_DeserializeFromString(::benchmark::State &state)
{
  // Get args form state
  BM_SerializeToString_config config{state};

  // set up a compiled model
  auto vm    = NewVM();
  auto model = vmSequentialModel(vm, config.sizes, config.activations, "mse", "adam");

  // serialise to string
  fetch::vm::Ptr<fetch::vm::String> serialized_model = model->SerializeToString();

  auto new_model = vmSequentialModel(vm);
  state.counters["charge"] =
      static_cast<double>(new_model->Estimator().DeserializeFromString(serialized_model));
  state.counters["StrLen"] = static_cast<double>(serialized_model->string().size());

  for (auto _ : state)
  {
    new_model->DeserializeFromString(serialized_model);
  }
}

//// (BM_SerializeToString_config) number_of_layers, input_size, hidden_1_size, ...., output_size,
/// hidden_1_activation, ...
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 10, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 10000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 100000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 1000000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1, 10000000, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_DeserializeFromString)->Args({2, 10, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 100, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 10000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 100000, 1, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 1000000, 1, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_DeserializeFromString)->Args({2, 1000, 1000, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 100, 100, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({2, 10, 10, 0})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_DeserializeFromString)
    ->Args({6, 1, 10, 100, 1000, 10000, 1, 0, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_DeserializeFromString)
    ->Args({5, 10000, 1000, 100, 10, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)
    ->Args({4, 1, 1, 1, 1, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_DeserializeFromString)
    ->Args({8, 1, 1, 1, 1, 1, 1, 1, 1})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_DeserializeFromString)
    ->Args({5, 100000, 1, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)
    ->Args({5, 1, 100000, 1, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)
    ->Args({5, 1, 1, 100000, 1, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)
    ->Args({5, 1, 1, 1, 100000, 1, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)
    ->Args({5, 1, 1, 1, 1, 100000, 0, 0, 0, 0})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_DeserializeFromString)->Args({3, 1, 1, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({3, 1, 10, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({3, 1, 100, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({3, 1, 1000, 1, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({3, 1, 10000, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({3, 1, 100000, 0, 0})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_DeserializeFromString)->Args({3, 1, 100000, 0, 0})->Unit(::benchmark::kMicrosecond);

}  // namespace

}  // namespace model
}  // namespace ml
}  // namespace benchmark
}  // namespace vm_modules
