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

using namespace fetch::vm;

namespace vm_modules {
namespace benchmark {
namespace ml {
namespace tensor {

void SetUp(std::shared_ptr<VM> &vm)
{
  using VMFactory = fetch::vm_modules::VMFactory;

  // setup the VM
  auto module = VMFactory::GetModule(fetch::vm_modules::VMFactory::USE_ALL);
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

struct BM_Tensor_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_Tensor_config(::benchmark::State const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(0));

    shape.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }
  }

  std::vector<SizeType> shape;  // layers input/output sizes
};

void BM_Construct(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  // Hidden as lambda tensor_constructor_charge_estimate
  state.counters["charge"] = static_cast<double>(999.9);

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    state.ResumeTiming();
    auto data = CreateTensor(vm, config.shape);
  }
}

BENCHMARK(BM_Construct)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Construct)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

Ptr<fetch::vm_modules::math::VMTensor> CreateTensorFromString(std::shared_ptr<VM> &vm,
                                                              std::string const &  str)
{
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(str);
}

struct BM_Tensor_String_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_Tensor_String_config(::benchmark::State const &state)
  {
    size = static_cast<SizeType>(state.range(0));
  }

  SizeType size;  // Number of elements
};

void BM_String_Construct(::benchmark::State &state)
{
  using VMPtr    = std::shared_ptr<VM>;
  using SizeType = fetch::math::SizeType;

  // Get args form state
  BM_Tensor_String_config config{state};
  state.counters["Size"] = static_cast<double>(config.size);

  // Hidden as lambda tensor_constructor_charge_estimate
  state.counters["charge"] = static_cast<double>(999.9);

  std::string str;
  for (SizeType i{0}; i < config.size; i++)
  {
    str += "1.0, ";
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    state.ResumeTiming();
    auto data = CreateTensorFromString(vm, str);
  }
}

BENCHMARK(BM_String_Construct)->Args({1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_String_Construct)->Args({10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_String_Construct)->Args({100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_String_Construct)->Args({1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_String_Construct)->Args({10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_String_Construct)->Args({100000})->Unit(::benchmark::kMicrosecond);

void BM_Fill(::benchmark::State &state)
{
  using VMPtr    = std::shared_ptr<VM>;
  using DataType = fetch::vm_modules::math::DataType;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  DataType val;
  auto     data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().Fill(val));

  for (auto _ : state)
  {
    data->Fill(val);
  }
}

BENCHMARK(BM_Fill)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({2, 1000, 1000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Fill)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_FillRandom(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().FillRandom());

  for (auto _ : state)
  {
    data->FillRandom();
  }
}

BENCHMARK(BM_FillRandom)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_FillRandom)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

struct BM_Reshape_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_Reshape_config(::benchmark::State const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(0));

    shape_from.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape_from.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }

    shape_to.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape_to.emplace_back(static_cast<SizeType>(state.range(1 + size_len + i)));
    }
  }

  std::vector<SizeType> shape_from;  // layers input/output sizes
  std::vector<SizeType> shape_to;    // layers input/output sizes
};

void BM_Reshape(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Reshape_config config{state};

  state.counters["PaddedSizeFrom"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape_from));

  state.counters["SizeFrom"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape_from));

  state.counters["PaddedSizeTo"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape_to));

  state.counters["SizeTo"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape_to));

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    auto data      = CreateTensor(vm, config.shape_from);
    auto new_shape = CreateArray(vm, config.shape_to);

    state.counters["charge"] = static_cast<double>(data->Estimator().Reshape(new_shape));

    state.ResumeTiming();
    data->Reshape(new_shape);
  }
}

BENCHMARK(BM_Reshape)->Args({3, 10, 1, 1, 1, 10, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 10, 1, 1, 1, 1, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 10, 1, 10, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 10, 1, 1, 1, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1, 10, 10, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1, 10, 1, 10, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Reshape)->Args({3, 1000000, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000000, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1000000, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1000000, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1, 1000000, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1, 1000000, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Reshape)->Args({3, 1, 1000, 1000, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1000, 1000, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000, 1, 1000, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000, 1, 1000, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000, 1000, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000, 1000, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);

// Same shape reshape
BENCHMARK(BM_Reshape)->Args({3, 100, 100, 100, 100, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000000, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1000000, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1, 1, 1000000, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Reshape)->Args({3, 1, 1000, 1000, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000, 1, 1000, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)->Args({3, 1000, 1000, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Reshape)->Args({3, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Reshape)
    ->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})
    ->Unit(::benchmark::kMicrosecond);

void BM_Transpose(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  state.counters["PaddedSizeBefore"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["PaddedSizeAfter"] = static_cast<double>(
      fetch::math::Tensor<float>::PaddedSizeFromShape({config.shape.at(1), config.shape.at(0)}));

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    auto data = CreateTensor(vm, config.shape);

    state.counters["charge"] = static_cast<double>(data->Estimator().Transpose());

    state.ResumeTiming();
    data->Transpose();
  }
}

BENCHMARK(BM_Transpose)->Args({2, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Transpose)->Args({2, 10, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 100, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1000, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 100, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 10000, 10000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Transpose)->Args({2, 10, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 100, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 10000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Transpose)->Args({2, 10000000, 1})->Unit(::benchmark::kMicrosecond);

struct BM_At_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_At_config(::benchmark::State const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(0));

    shape.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }

    indices.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      indices.emplace_back(static_cast<SizeType>(state.range(1 + size_len + i)));
    }
  }

  std::vector<SizeType> shape;    // layers input/output sizes
  std::vector<SizeType> indices;  // layers input/output sizes
};

void BM_At(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_At_config config{state};

  state.counters["PaddedSizeFrom"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["SizeFrom"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data      = CreateTensor(vm, config.shape);
  auto new_shape = CreateArray(vm, config.indices);

  for (auto _ : state)
  {
    state.PauseTiming();

    switch (config.shape.size())
    {
    case 1:
      state.counters["charge"] = static_cast<double>(data->Estimator().AtOne(config.indices.at(0)));
      state.ResumeTiming();
      data->At(config.indices.at(0));
      state.PauseTiming();
      break;

    case 2:
      state.counters["charge"] =
          static_cast<double>(data->Estimator().AtTwo(config.indices.at(0), config.indices.at(1)));
      state.ResumeTiming();
      data->At(config.indices.at(0), config.indices.at(1));
      state.PauseTiming();
      break;

    case 3:
      state.counters["charge"] = static_cast<double>(data->Estimator().AtThree(
          config.indices.at(0), config.indices.at(1), config.indices.at(2)));
      state.ResumeTiming();
      data->At(config.indices.at(0), config.indices.at(1), config.indices.at(2));
      state.PauseTiming();
      break;

    case 4:
      state.counters["charge"] = static_cast<double>(data->Estimator().AtFour(
          config.indices.at(0), config.indices.at(1), config.indices.at(2), config.indices.at(3)));
      state.ResumeTiming();
      data->At(config.indices.at(0), config.indices.at(1), config.indices.at(2),
               config.indices.at(3));
      state.PauseTiming();
      break;
    }
  }
}

BENCHMARK(BM_At)->Args({4, 30, 30, 30, 30, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({4, 30, 30, 30, 30, 29, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({4, 30, 30, 30, 30, 1, 29, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({4, 30, 30, 30, 30, 1, 1, 29, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({4, 30, 30, 30, 30, 1, 1, 1, 29})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({4, 30, 30, 30, 30, 29, 29, 29, 29})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_At)->Args({3, 100, 100, 100, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({3, 100, 100, 100, 99, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({3, 100, 100, 100, 1, 99, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({3, 100, 100, 100, 1, 1, 99})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_At)->Args({3, 100, 100, 100, 99, 99, 99})->Unit(::benchmark::kMicrosecond);

struct BM_SetAt_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_SetAt_config(::benchmark::State const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(0));

    shape.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape.emplace_back(static_cast<SizeType>(state.range(1 + i)));
    }

    indices.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      indices.emplace_back(static_cast<SizeType>(state.range(1 + size_len + i)));
    }
  }

  std::vector<SizeType> shape;    // layers input/output sizes
  std::vector<SizeType> indices;  // layers input/output sizes
};

void BM_SetAt(::benchmark::State &state)
{
  using VMPtr    = std::shared_ptr<VM>;
  using DataType = fetch::vm_modules::math::DataType;

  // Get args form state
  BM_SetAt_config config{state};

  state.counters["PaddedSizeFrom"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["SizeFrom"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data      = CreateTensor(vm, config.shape);
  auto new_shape = CreateArray(vm, config.indices);

  DataType val;

  for (auto _ : state)
  {
    state.PauseTiming();

    switch (config.shape.size())
    {
    case 1:
      state.counters["charge"] =
          static_cast<double>(data->Estimator().SetAtOne(config.indices.at(0), val));
      state.ResumeTiming();
      data->SetAt(config.indices.at(0), val);
      state.PauseTiming();
      break;

    case 2:
      state.counters["charge"] = static_cast<double>(
          data->Estimator().SetAtTwo(config.indices.at(0), config.indices.at(1), val));
      state.ResumeTiming();
      data->SetAt(config.indices.at(0), config.indices.at(1), val);
      state.PauseTiming();
      break;

    case 3:
      state.counters["charge"] = static_cast<double>(data->Estimator().SetAtThree(
          config.indices.at(0), config.indices.at(1), config.indices.at(2), val));
      state.ResumeTiming();
      data->SetAt(config.indices.at(0), config.indices.at(1), config.indices.at(2), val);
      state.PauseTiming();
      break;

    case 4:
      state.counters["charge"] = static_cast<double>(
          data->Estimator().SetAtFour(config.indices.at(0), config.indices.at(1),
                                      config.indices.at(2), config.indices.at(3), val));
      state.ResumeTiming();
      data->SetAt(config.indices.at(0), config.indices.at(1), config.indices.at(2),
                  config.indices.at(3), val);
      state.PauseTiming();
      break;
    }
  }
}

BENCHMARK(BM_SetAt)->Args({4, 30, 30, 30, 30, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({4, 30, 30, 30, 30, 29, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({4, 30, 30, 30, 30, 1, 29, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({4, 30, 30, 30, 30, 1, 1, 29, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({4, 30, 30, 30, 30, 1, 1, 1, 29})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({4, 30, 30, 30, 30, 29, 29, 29, 29})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_SetAt)->Args({3, 100, 100, 100, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({3, 100, 100, 100, 99, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({3, 100, 100, 100, 1, 99, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({3, 100, 100, 100, 1, 1, 99})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_SetAt)->Args({3, 100, 100, 100, 99, 99, 99})->Unit(::benchmark::kMicrosecond);

void BM_ToString(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().ToString());

  for (auto _ : state)
  {
    data->ToString();
  }
}

BENCHMARK(BM_ToString)->Args({2, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 10000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 100, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 10, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ToString)->Args({2, 1, 10})->Unit(::benchmark::kMicrosecond);

void BM_FromString(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);
  auto str  = data->ToString();

  state.counters["StrLen"] = static_cast<double>(str->string().size());
  state.counters["charge"] = static_cast<double>(data->Estimator().FromString(str));

  for (auto _ : state)
  {
    data->FromString(str);
  }
}

BENCHMARK(BM_FromString)->Args({2, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 10000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 100, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 10, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FromString)->Args({2, 1, 10})->Unit(::benchmark::kMicrosecond);

void BM_Min(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().Min());

  for (auto _ : state)
  {
    data->Min();
  }
}

BENCHMARK(BM_Min)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Min)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_Max(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().Max());

  for (auto _ : state)
  {
    data->Max();
  }
}

BENCHMARK(BM_Max)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Max)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_Sum(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().Sum());

  for (auto _ : state)
  {
    data->Sum();
  }
}

BENCHMARK(BM_Sum)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Sum)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

struct BM_ArgMax_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_ArgMax_config(::benchmark::State const &state)
  {
    auto size_len = static_cast<SizeType>(state.range(1));

    index = static_cast<SizeType>(state.range(0));

    shape.reserve(size_len);
    for (SizeType i{0}; i < size_len; ++i)
    {
      shape.emplace_back(static_cast<SizeType>(state.range(2 + i)));
    }
  }

  std::vector<SizeType> shape;  // layers input/output sizes
  SizeType              index;  // layers input/output sizes
};

void BM_ArgMax(::benchmark::State &state)
{
  using VMPtr    = std::shared_ptr<VM>;
  using SizeType = fetch::math::SizeType;

  // Get args form state
  BM_ArgMax_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  state.counters["SizeAtIndex"] = static_cast<double>(config.shape.at(config.index));

  std::vector<SizeType> ret_shape = config.shape;

  ret_shape.at(config.index) = 1;
  // ret_shape.erase(ret_shape.begin() + int(config.index));

  state.counters["RetPaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(ret_shape));

  state.counters["RetSize"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(ret_shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().ArgMax(config.index));

  for (auto _ : state)
  {
    data->ArgMax(config.index);
  }
}

BENCHMARK(BM_ArgMax)->Args({0, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({1, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({1, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 3, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({1, 3, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({1, 3, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({1, 3, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({2, 3, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 3, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 3, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 7, 100000, 2, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 2, 100000, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 2, 2, 100000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 2, 2, 2, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 2, 2, 2, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 2, 2, 2, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 2, 2, 2, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({2, 7, 100000, 2, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 7, 2, 100000, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 7, 2, 2, 100000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 7, 2, 2, 2, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 7, 2, 2, 2, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 7, 2, 2, 2, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 7, 2, 2, 2, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({4, 7, 100000, 2, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({4, 7, 2, 100000, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({4, 7, 2, 2, 100000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({4, 7, 2, 2, 2, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({4, 7, 2, 2, 2, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({4, 7, 2, 2, 2, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({4, 7, 2, 2, 2, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({6, 7, 100000, 2, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({6, 7, 2, 100000, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({6, 7, 2, 2, 100000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({6, 7, 2, 2, 2, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({6, 7, 2, 2, 2, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({6, 7, 2, 2, 2, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({6, 7, 2, 2, 2, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({1, 3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 4, 100, 100, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({1, 4, 100, 100, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 4, 100, 100, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({3, 4, 100, 100, 100, 100})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({1, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({2, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({3, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({4, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({5, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({6, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({7, 8, 10, 10, 10, 10, 10, 10, 10, 10})->Unit(::benchmark::kMicrosecond);

void BM_ArgMaxNoIndices(::benchmark::State &state)
{
  using VMPtr    = std::shared_ptr<VM>;
  using SizeType = fetch::math::SizeType;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  state.counters["SizeAtIndex"] = static_cast<double>(config.shape.at(0));

  std::vector<SizeType> ret_shape = config.shape;

  ret_shape.at(0) = 1;
  //  ret_shape.erase(ret_shape.begin() + int(0));

  state.counters["RetPaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(ret_shape));

  state.counters["RetSize"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(ret_shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().ArgMaxNoIndices());

  for (auto _ : state)
  {
    data->ArgMaxNoIndices();
  }
}

BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 100000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 100000, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 2, 100000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 2, 2, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 2, 2, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 2, 2, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 2, 2, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 100000, 2, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 2, 100000, 2, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 2, 2, 100000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 2, 2, 2, 100000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 2, 2, 2, 2, 100000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 2, 2, 2, 2, 2, 100000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 2, 2, 2, 2, 2, 2, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 100000, 2, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 100000, 1000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 2, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 100000, 2})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 2, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1000, 2})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 100, 100, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 2, 2, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 2, 1000, 2, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1000, 2, 2, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1000, 2, 1000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1000, 1000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 2, 1000, 2, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 2, 1000, 1000, 2})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)
    ->Args({10, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 1000000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 2, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 1000000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 2, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 2, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1000000, 2, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 1000000, 2, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 2, 1000000, 2, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 2, 2, 1000000, 2})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 2, 2, 2, 2, 1000000})->Unit(::benchmark::kMicrosecond);

struct BM_Dot_config
{
  using SizeType = fetch::math::SizeType;

  explicit BM_Dot_config(::benchmark::State const &state)
  {
    x = static_cast<SizeType>(state.range(0));
    y = static_cast<SizeType>(state.range(1));
    c = static_cast<SizeType>(state.range(2));
  }

  SizeType x;  // A.shape(0)
  SizeType y;  // B.shape(1)
  SizeType c;  // A.shape(1) and B.shape(0)
};

void BM_Dot(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Dot_config config{state};

  state.counters["PaddedSizeA"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape({config.x, config.c}));

  state.counters["SizeA"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape({config.x, config.c}));

  state.counters["PaddedSizeB"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape({config.c, config.y}));

  state.counters["SizeB"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape({config.c, config.y}));

  VMPtr vm;
  SetUp(vm);

  auto data_a = CreateTensor(vm, {config.x, config.c});
  auto data_b = CreateTensor(vm, {config.c, config.y});

  state.counters["charge"] = static_cast<double>(data_a->Estimator().Dot(data_b));

  for (auto _ : state)
  {
    data_a->Dot(data_b);
  }
}

BENCHMARK(BM_Dot)->Args({1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({10, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({100, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({10000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({100000000, 1, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Dot)->Args({1, 10, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 100, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 10000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 10000000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Dot)->Args({1, 1, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Dot)->Args({10, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({100, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Dot)->Args({10, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({100, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 10, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 100, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 1000, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 1000, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({100, 1000, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({10, 1000, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 100, 100})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({1000, 10, 10})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({100, 100, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Dot)->Args({10, 10, 1000})->Unit(::benchmark::kMicrosecond);

void BM_IsEqual(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  state.counters["charge"] =
      static_cast<double>(data->Estimator().IsEqualChargeEstimator(data, data_2));

  for (auto _ : state)
  {
    data->IsEqual(data, data_2);
  }
}

BENCHMARK(BM_IsEqual)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsEqual)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsEqual)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_IsNotEqual(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  state.counters["charge"] =
      static_cast<double>(data->Estimator().IsNotEqualChargeEstimator(data, data_2));

  for (auto _ : state)
  {
    data->IsNotEqual(data, data_2);
  }
}

BENCHMARK(BM_IsNotEqual)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_IsNotEqual)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_IsNotEqual)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_Negate(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  Ptr<Object> operand      = data;
  state.counters["charge"] = static_cast<double>(data->Estimator().NegateChargeEstimator(operand));

  for (auto _ : state)
  {
    data->Negate(operand);
  }
}

BENCHMARK(BM_Negate)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Negate)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Negate)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_Add(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  Ptr<Object> operand_1 = data;
  Ptr<Object> operand_2 = data_2;

  state.counters["charge"] =
      static_cast<double>(data->Estimator().AddChargeEstimator(operand_1, operand_2));

  for (auto _ : state)
  {
    data->Add(operand_1, operand_2);
  }
}

BENCHMARK(BM_Add)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Add)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Add)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_Multiply(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  Ptr<Object> operand_1 = data;
  Ptr<Object> operand_2 = data_2;

  state.counters["charge"] =
      static_cast<double>(data->Estimator().MultiplyChargeEstimator(operand_1, operand_2));

  for (auto _ : state)
  {
    data->Multiply(operand_1, operand_2);
  }
}

BENCHMARK(BM_Multiply)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Multiply)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Multiply)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_Divide(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  Ptr<Object> operand_1 = data;
  Ptr<Object> operand_2 = data_2;

  state.counters["charge"] =
      static_cast<double>(data->Estimator().DivideChargeEstimator(operand_1, operand_2));

  for (auto _ : state)
  {
    data->Divide(operand_1, operand_2);
  }
}

BENCHMARK(BM_Divide)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Divide)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Divide)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_InplaceAdd(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  Ptr<Object> operand_1 = data;
  Ptr<Object> operand_2 = data_2;

  state.counters["charge"] =
      static_cast<double>(data->Estimator().InplaceAddChargeEstimator(operand_1, operand_2));

  for (auto _ : state)
  {
    data->InplaceAdd(operand_1, operand_2);
  }
}

BENCHMARK(BM_InplaceAdd)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceAdd)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceAdd)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_InplaceSubtract(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  Ptr<Object> operand_1 = data;
  Ptr<Object> operand_2 = data_2;

  state.counters["charge"] =
      static_cast<double>(data->Estimator().InplaceSubtractChargeEstimator(operand_1, operand_2));

  for (auto _ : state)
  {
    data->InplaceSubtract(operand_1, operand_2);
  }
}

BENCHMARK(BM_InplaceSubtract)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceSubtract)
    ->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceSubtract)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_InplaceMultiply(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  Ptr<Object> operand_1 = data;
  Ptr<Object> operand_2 = data_2;

  state.counters["charge"] =
      static_cast<double>(data->Estimator().InplaceMultiplyChargeEstimator(operand_1, operand_2));

  for (auto _ : state)
  {
    data->InplaceMultiply(operand_1, operand_2);
  }
}

BENCHMARK(BM_InplaceMultiply)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceMultiply)
    ->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceMultiply)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_InplaceDivide(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data   = CreateTensor(vm, config.shape);
  auto data_2 = CreateTensor(vm, config.shape);

  Ptr<Object> operand_1 = data;
  Ptr<Object> operand_2 = data_2;

  state.counters["charge"] =
      static_cast<double>(data->Estimator().InplaceDivideChargeEstimator(operand_1, operand_2));

  for (auto _ : state)
  {
    data->InplaceDivide(operand_1, operand_2);
  }
}

BENCHMARK(BM_InplaceDivide)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_InplaceDivide)
    ->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_InplaceDivide)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_Copy(::benchmark::State &state)
{
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_Tensor_config config{state};

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(config.shape));

  state.counters["Size"] =
      static_cast<double>(fetch::math::Tensor<float>::SizeFromShape(config.shape));

  VMPtr vm;
  SetUp(vm);

  auto data = CreateTensor(vm, config.shape);

  state.counters["charge"] = static_cast<double>(data->Estimator().Copy());

  for (auto _ : state)
  {
    data->Copy();
  }
}

BENCHMARK(BM_Copy)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({3, 300, 300, 300})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 100000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 100000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1000, 1, 100000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1000, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 10000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_Copy)->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Copy)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

}  // namespace tensor
}  // namespace ml
}  // namespace benchmark
}  // namespace vm_modules
