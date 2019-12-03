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

namespace vm_modules {
namespace benchmark {
namespace ml {
namespace model {

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

void BM_AddLayer(::benchmark::State &state)
{
  using VMPtr        = std::shared_ptr<VM>;
  using SizeRef      = fetch::math::SizeType const &;
  using SizeType     = fetch::math::SizeType;
  using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    auto model = CreateSequentialModel(vm);

    if (state.range(2))
    {
      state.ResumeTiming();
      model->AddLayer<SizeRef, SizeRef>(
          CreateString(vm, "dense"), static_cast<SizeType>(state.range(0)),
          static_cast<SizeType>(state.range(1)));  // input_size, output_size
    }
    else
    {
      state.ResumeTiming();
      model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
          CreateString(vm, "dense"), static_cast<SizeType>(state.range(0)),
          static_cast<SizeType>(state.range(1)),
          CreateString(vm, "relu"));  // input_size, output_size
    }
  }
}
/*
// input_size, output_size, activation
BENCHMARK(BM_AddLayer)->Args({1, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1000, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1000, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1, 10000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({1, 100000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({100000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({200, 200, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({2000, 20, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({3000, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_AddLayer)->Args({10, 3000, false})->Unit(::benchmark::kMicrosecond);
*/
void BM_Predict(::benchmark::State &state)
{
  using VMPtr        = std::shared_ptr<VM>;
  using SizeRef      = fetch::math::SizeType const &;
  using SizeType     = fetch::math::SizeType;
  using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    SizeType subset_size = static_cast<SizeType>(state.range(0));
    SizeType input_size  = static_cast<SizeType>(state.range(2));
    SizeType args_size   = static_cast<SizeType>(state.range(1) + 2);

    // set up data and labels
    std::vector<uint64_t> data_shape{input_size, subset_size};
    auto                  data = CreateTensor(vm, data_shape);

    auto model = CreateSequentialModel(vm);

    for (SizeType i{2}; i < (args_size - 1); i++)
    {
      if (state.range(args_size + i - 2))
      {

        model->AddLayer<SizeRef, SizeRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)));  // input_size, output_size
      }
      else
      {
        model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)),
            CreateString(vm, "relu"));  // input_size, output_size
      }
    }

    model->CompileSequential(CreateString(vm, "mse"), CreateString(vm, "adam"));
    state.ResumeTiming();
    auto res = model->Predict(data);
  }
}
/*
// batch_size, number_of_layers, input_size, hidden_1_size, ...., output_size, activation_1,....
BENCHMARK(BM_Predict)
    ->Args({1, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({2, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({4, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({8, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({16, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({32, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({64, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({1, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({2, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({4, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({8, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({16, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({32, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({64, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({128, 4, 1, 1, 1, 1, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 4, 1, 1, 1, 1, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 4, 1, 1, 1, 1, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({1024, 4, 1, 1, 1, 1, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({2048, 4, 1, 1, 1, 1, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({128, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({1024, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({2048, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({128, 5, 10000, 1, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 10000, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 1, 10000, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 1, 1, 10000, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({128, 5, 1, 1, 1, 1, 10000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({512, 5, 10000, 1, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 10000, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 1, 10000, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 1, 1, 10000, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1, 1, 1, 1, 10000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 2, 1, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 10000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1, 100000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 3, 1, 1, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 10, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 100, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 10000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 3, 1, 100000, 1, false, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 2, 10, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 100, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 10000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 100000, 1, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)->Args({1, 2, 10000, 10000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 100, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)->Args({1, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Predict)
    ->Args({128, 5, 1000, 1000, 1000, 1000, 1000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({256, 5, 1000, 1000, 1000, 1000, 1000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Predict)
    ->Args({512, 5, 1000, 1000, 1000, 1000, 1000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
*/
void BM_Compile(::benchmark::State &state)
{
  using VMPtr        = std::shared_ptr<VM>;
  using SizeRef      = fetch::math::SizeType const &;
  using SizeType     = fetch::math::SizeType;
  using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    SizeType args_size = static_cast<SizeType>(state.range(0) + 1);

    // set up data and labels
    auto model = CreateSequentialModel(vm);

    for (SizeType i{1}; i < args_size - 1; i++)
    {
      if (state.range(args_size + i - 1))
      {

        model->AddLayer<SizeRef, SizeRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)));  // input_size, output_size
      }
      else
      {
        model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)),
            CreateString(vm, "relu"));  // input_size, output_size
      }
    }

    state.ResumeTiming();
    model->CompileSequential(CreateString(vm, "mse"), CreateString(vm, "adam"));
  }
}

// number_of_layers, input_size, hidden_1_size, ...., output_size, activation_1,....
/*
BENCHMARK(BM_Compile)->Args({2, 1, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 10000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 100000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 1000000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 10000000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1, 100000000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({2, 10, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 100, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 10000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 100000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1000000, 1, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({2, 10000, 10000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 100, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({2, 10, 10, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)
    ->Args({6, 1, 10, 100, 1000, 10000, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)
    ->Args({5, 10000, 1000, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({4, 1, 1, 1, 1, false, false, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({8, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)
    ->Args({5, 10000, 1, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)
    ->Args({5, 1, 10000, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)
    ->Args({5, 1, 1, 10000, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)
    ->Args({5, 1, 1, 1, 10000, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)
    ->Args({5, 1, 1, 1, 1, 10000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)->Args({3, 1, 1, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 10, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 100, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 10000, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Compile)->Args({3, 1, 100000, 1, false, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Compile)
    ->Args({5, 1000, 1000, 1000, 1000, 1000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
*/
void BM_Fit(::benchmark::State &state)
{
  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);

  using VMPtr        = std::shared_ptr<VM>;
  using SizeRef      = fetch::math::SizeType const &;
  using SizeType     = fetch::math::SizeType;
  using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    SizeType subset_size = static_cast<SizeType>(state.range(0));
    SizeType batch_size  = static_cast<SizeType>(state.range(1));
    SizeType input_size  = static_cast<SizeType>(state.range(3));
    SizeType args_size   = static_cast<SizeType>(state.range(2) + 3);
    SizeType label_size  = static_cast<SizeType>(state.range(args_size - 1));

    // set up data and labels
    std::vector<uint64_t> data_shape{input_size, subset_size};
    std::vector<uint64_t> label_shape{label_size, subset_size};

    auto data  = CreateTensor(vm, data_shape);
    auto label = CreateTensor(vm, label_shape);

    auto model = CreateSequentialModel(vm);

    for (SizeType i{3}; i < args_size - 1; i++)
    {
      if (state.range(args_size + i - 3))
      {
        model->AddLayer<SizeRef, SizeRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)));  // input_size, output_size
      }
      else
      {
        model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)),
            CreateString(vm, "relu"));  // input_size, output_size
      }
    }

    model->CompileSequential(CreateString(vm, "mse"), CreateString(vm, "adam"));
    state.ResumeTiming();

    model->Fit(data, label, batch_size);
  }
}

/*
// n_datapoints, batch_size, num_layers, in_size, hidden_1_size, ...., out_size, activation_1,....

// MNIST
BENCHMARK(BM_Fit)->Args({32, 32, 3, 784,100, 10, true,true})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({320, 32, 3, 784,100, 10, true,true})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({3200, 32, 3, 784,100, 10, true,true})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1000, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10000, 2, 10, 10, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1000, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10000, 2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1000, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10000, 2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 1, 3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1000, 1, 3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 1, 3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 10, 3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10000, 100, 3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 1000, 3, 1, 1000, 1, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 10000, 3, 1, 1000, 1, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)
    ->Args({10, 1, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({100, 1, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({1000, 1, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 1, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 10, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 100, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 1000, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10000, 10000, 5, 10, 100, 1, 100, 10, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({1, 1, 3, 1, 1000000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1, 1, 2, 1000000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1, 1, 2, 1, 1000000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({1, 1, 2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 1, 3, 1, 1000000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 1, 2, 1000000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 1, 2, 1, 1000000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 1, 2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({10, 10, 3, 1, 1000000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 10, 2, 1000000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 10, 2, 1, 1000000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({10, 10, 2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)->Args({100, 10, 3, 1, 1000000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 10, 2, 1000000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 10, 2, 1, 1000000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)->Args({100, 10, 2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fit)
    ->Args({1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({10, 10, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({100, 10, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fit)
    ->Args({100, 100, 8, 1, 1, 1, 1, 1, 1, 1, 1, false, false, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
*/

/// SERIALISATION BENCHMARKS ///

// 1. benchmark model->SerialiseToString
void BM_Serialise(::benchmark::State &state)
{
  using VMPtr        = std::shared_ptr<VM>;
  using SizeRef      = fetch::math::SizeType const &;
  using SizeType     = fetch::math::SizeType;
  using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

  SizeType args_size = static_cast<SizeType>(state.range(0) + 1);

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    // set up data and labels
    auto model = CreateSequentialModel(vm);

    for (SizeType i{1}; i < (args_size - 1); i++)
    {
      if (state.range(args_size + i - 1))
      {
        model->AddLayer<SizeRef, SizeRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)));  // input_size, output_size
      }
      else
      {
        model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
            CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
            static_cast<SizeType>(state.range(i + 1)),
            CreateString(vm, "relu"));  // input_size, output_size
      }
    }
    model->CompileSequential(CreateString(vm, "mse"), CreateString(vm, "adam"));

    state.ResumeTiming();
    model->SerializeToString();
  }
}
/*
//// number_of_layers, input_size, hidden_1_size, ...., output_size, hidden_1_activation, ...
        BENCHMARK(BM_Serialise)->Args({2, 1, 1, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 1, 10, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 1, 100, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 1, 10000, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 1, 100000, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 1, 1000000, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 1, 10000000, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 1, 100000000, false})->Unit(::benchmark::kMicrosecond);

        BENCHMARK(BM_Serialise)->Args({2, 10, 1, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 100, 1, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 10000, 1, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 100000, 1, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 1000000, 1, false})->Unit(::benchmark::kMicrosecond);

        //BENCHMARK(BM_Serialise)->Args({2, 10000, 10000, false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 100, 100, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({2, 10, 10, false})->Unit(::benchmark::kMicrosecond);

        BENCHMARK(BM_Serialise)->Args({6, 1, 10, 100, 100, 100, 1, false, false, false, false,
false})->Unit(::benchmark::kMicrosecond);

        BENCHMARK(BM_Serialise)->Args({5, 100, 100, 100, 10, 1, false, false, false,
false})->Unit(::benchmark::kMicrosecond); BENCHMARK(BM_Serialise)->Args({4, 1, 1, 1, 1, false,
false, false})->Unit(::benchmark::kMicrosecond);

        BENCHMARK(BM_Serialise)->Args({8, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);

        BENCHMARK(BM_Serialise)
                ->Args({5, 1000, 1, 1, 1, 1, false, false, false, false})
                ->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)
                ->Args({5, 1, 1000, 1, 1, 1, false, false, false, false})
                ->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)
                ->Args({5, 1, 1, 1000, 1, 1, false, false, false, false})
                ->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)
                ->Args({5, 1, 1, 1, 1000, 1, false, false, false, false})
                ->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)
                ->Args({5, 1, 1, 1, 1, 1000, false, false, false, false})
                ->Unit(::benchmark::kMicrosecond);

        BENCHMARK(BM_Serialise)->Args({3, 1, 1, 1, false, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({3, 1, 10, 1, false, false})->Unit(::benchmark::kMicrosecond);
        BENCHMARK(BM_Serialise)->Args({3, 1, 100, 1, false,
false})->Unit(::benchmark::kMicrosecond); BENCHMARK(BM_Serialise)->Args({3, 1, 1000, 1, false,
false})->Unit(::benchmark::kMicrosecond); BENCHMARK(BM_Serialise)->Args({3, 1, 10000, false,
false})->Unit(::benchmark::kMicrosecond);
        //BENCHMARK(BM_Serialise)->Args({3, 1, 100000, 1, false,
false})->Unit(::benchmark::kMicrosecond);
*/

// 2. benchmark model->DeserialiseFromString
void BM_Deserialise(::benchmark::State &state)
{
  using VMPtr        = std::shared_ptr<VM>;
  using SizeRef      = fetch::math::SizeType const &;
  using SizeType     = fetch::math::SizeType;
  using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

  SizeType args_size = static_cast<SizeType>(state.range(0) + 1);

  // state.PauseTiming();
  VMPtr vm;
  SetUp(vm);

  // set up data and labels
  auto model = CreateSequentialModel(vm);

  for (SizeType i{1}; i < (args_size - 1); i++)
  {
    if (state.range(args_size + i - 1))
    {
      model->AddLayer<SizeRef, SizeRef>(
          CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
          static_cast<SizeType>(state.range(i + 1)));  // input_size, output_size
    }
    else
    {
      model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
          CreateString(vm, "dense"), static_cast<SizeType>(state.range(i)),
          static_cast<SizeType>(state.range(i + 1)),
          CreateString(vm, "relu"));  // input_size, output_size
    }
  }
  model->CompileSequential(CreateString(vm, "mse"), CreateString(vm, "adam"));
  fetch::vm::Ptr<fetch::vm::String> serialised_model = model->SerializeToString();

  state.counters["StrLen"] = static_cast<double>(serialised_model->string().size());

  for (auto _ : state)
  {
    // state.ResumeTiming();
    model->DeserializeFromString(serialised_model);
  }
}

//// number_of_layers, input_size, hidden_1_size, ...., output_size, hidden_1_activation, ...
BENCHMARK(BM_Deserialise)->Args({2, 1, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1, 10, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1, 10000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1, 100000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1, 1000000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1, 10000000, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Deserialise)->Args({2, 10, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 100, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 10000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 100000, 1, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 1000000, 1, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Deserialise)->Args({2, 1000, 1000, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 100, 100, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({2, 10, 10, false})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Deserialise)
    ->Args({6, 1, 10, 100, 100, 100, 1, false, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Deserialise)
    ->Args({5, 100, 100, 100, 10, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)
    ->Args({4, 1, 1, 1, 1, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Deserialise)->Args({8, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Deserialise)
    ->Args({5, 1000, 1, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)
    ->Args({5, 1, 1000, 1, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)
    ->Args({5, 1, 1, 1000, 1, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)
    ->Args({5, 1, 1, 1, 1000, 1, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)
    ->Args({5, 1, 1, 1, 1, 1000, false, false, false, false})
    ->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Deserialise)->Args({3, 1, 1, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({3, 1, 10, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({3, 1, 100, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({3, 1, 1000, 1, false, false})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Deserialise)->Args({3, 1, 10000, false, false})->Unit(::benchmark::kMicrosecond);
// BENCHMARK(BM_Deserialise)->Args({3, 1, 100000, 1, false,
// false})->Unit(::benchmark::kMicrosecond);

}  // namespace model
}  // namespace ml
}  // namespace benchmark
}  // namespace vm_modules
