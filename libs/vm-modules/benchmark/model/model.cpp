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

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gmock/gmock.h"

#include <sstream>

#include "benchmark/benchmark.h"

#include <vector>

using namespace fetch::vm;

void SetUp(std::shared_ptr<VM> &vm)
{
  using VMFactory = fetch::vm_modules::VMFactory;

  // setup the VM
  auto module = VMFactory::GetModule(fetch::vm_modules::VMFactory::USE_SMART_CONTRACTS);
  vm          = std::make_shared<VM>(module.get());
}

Ptr<String> CreateString(std::shared_ptr<VM> &vm, std::string const &str)
{
  return Ptr<String>{new String{vm.get(), str}};
}

Ptr<Array<uint64_t>> CreateArray(std::shared_ptr<VM> &vm, std::vector<uint64_t> const &values)
{
  std::size_t          size = values.size();
  Ptr<Array<uint64_t>> array =
      vm->CreateNewObject<Array<uint64_t>>(vm->GetTypeId<uint64_t>(), static_cast<int32_t>(size));
  ;

  for (std::size_t i{0}; i < size; ++i)
  {
    array->elements[i] = values[i];
  }

  return array;
}

Ptr<fetch::vm_modules::math::VMTensor> CreateTensor(std::shared_ptr<VM> &        vm,
                                                    std::vector<uint64_t> const &shape)
{
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(shape);
}

Ptr<fetch::vm_modules::ml::model::VMModel> CreateSequentialModel(std::shared_ptr<VM> &vm)
{
  auto model_category = CreateString(vm, "sequential");
  return vm->CreateNewObject<fetch::vm_modules::ml::model::VMModel>(model_category);
}

template <typename T, int S, int I, int O>
void BM_AddLayer(benchmark::State &state)
{
  //    using VMModel        = fetch::vm_modules::ml::model::VMModel;
  //    using VMTensor       = fetch::vm_modules::math::VMTensor;
  //    using ModelEstimator = fetch::vm_modules::ml::model::ModelEstimator;
  using VMPtr = std::shared_ptr<VM>;


  /*
  // set up a model
  auto model = CreateSequentialModel();
  model->LayerAddDenseActivation(CreateString("dense"), 10, 10, CreateString("relu")); //
  input_size, hidden_1_size model->LayerAddDenseActivation(CreateString("dense"), 10, 10,
  CreateString("relu")); // hidden_1_size, hidden_2_size model->LayerAddDense(CreateString("dense"),
  10, 1);  // hidden_2_size, label_size model->CompileSequential(CreateString("mse"),
  CreateString("adam"));

  // train the model
  model->Fit(data, label, 32); // batch_size

  // get loss value
  auto loss* = model->Evaluate();

  // make a prediction
  Ptr<VMTensor> prediction = model->Predict(data);
  */

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    // set up data and labels
    std::vector<uint64_t> data_shape{I, S};
    std::vector<uint64_t> label_shape{O, S};
    auto                  data  = CreateTensor(vm, data_shape);
    auto                  label = CreateTensor(vm, label_shape);

    auto model = CreateSequentialModel(vm);
    state.ResumeTiming();

    model->LayerAddDense(CreateString(vm, "dense"), I, O);  // input_size, hidden_1_size

   // state.PauseTiming();
   // vm->~VM();
   // state.ResumeTiming();
  }
}

BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 10, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 1000, 1000)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 100, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 1000, 10)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 10, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 10, 1000)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 100, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 100, 100, 1000)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 1, 1000, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 1000, 1, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 200, 200, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 2000, 20, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 3000, 10, 100)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_AddLayer, float, 10, 3000, 100)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
