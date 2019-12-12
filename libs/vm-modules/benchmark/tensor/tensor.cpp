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

BENCHMARK(BM_Construct)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
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
BENCHMARK(BM_Construct)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Construct)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

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

BENCHMARK(BM_Fill)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Fill)->Args({3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
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
BENCHMARK(BM_Fill)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
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

BENCHMARK(BM_FillRandom)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_FillRandom)->Args({3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
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
BENCHMARK(BM_FillRandom)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
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

BENCHMARK(BM_Min)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Min)->Args({3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
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
BENCHMARK(BM_Min)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
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

BENCHMARK(BM_Max)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Max)->Args({3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
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
BENCHMARK(BM_Max)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
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

BENCHMARK(BM_Sum)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_Sum)->Args({3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
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
BENCHMARK(BM_Sum)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
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
    auto size_len = static_cast<SizeType>(state.range(0));

    index = static_cast<SizeType>(state.range(1));

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
  using VMPtr = std::shared_ptr<VM>;

  // Get args form state
  BM_ArgMax_config config{state};

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
    data->ArgMax(config.index);
  }
}

BENCHMARK(BM_ArgMax)->Args({0, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMax)->Args({0, 10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMax)->Args({0, 5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

void BM_ArgMaxNoIndices(::benchmark::State &state)
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
    data->ArgMaxNoIndices();
  }
}

BENCHMARK(BM_ArgMaxNoIndices)->Args({1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({6, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 100000, 1, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 1, 100000, 1, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 1, 1, 100000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 1, 1, 1, 100000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 1, 1, 1, 1, 100000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 1, 1, 1, 1, 1, 100000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({7, 1, 1, 1, 1, 1, 1, 100000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 10000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 1000, 10000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 10000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 1, 10000000})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1000, 1, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1000, 1, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1000, 1000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1, 1000, 1, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({4, 1, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);

BENCHMARK(BM_ArgMaxNoIndices)
    ->Args({10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})
    ->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({2, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1000, 1000})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({3, 1000, 1000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1000000, 1, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 1000000, 1, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 1, 1000000, 1, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 1, 1, 1000000, 1})->Unit(::benchmark::kMicrosecond);
BENCHMARK(BM_ArgMaxNoIndices)->Args({5, 1, 1, 1, 1, 1000000})->Unit(::benchmark::kMicrosecond);

}  // namespace tensor
}  // namespace ml
}  // namespace benchmark
}  // namespace vm_modules
