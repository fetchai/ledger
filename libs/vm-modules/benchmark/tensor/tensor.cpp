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

void BM_Construct(::benchmark::State &state)
{
  using VMPtr      = std::shared_ptr<VM>;
  using SizeType   = fetch::math::SizeType;
  using SizeVector = std::vector<SizeType>;

  SizeVector shape{};

  SizeType elements = static_cast<SizeType>(state.range(0));

  for (SizeType i{1}; i < elements + 1; i++)
  {
    shape.push_back(static_cast<SizeType>(state.range(i)));
  }

  state.counters["PaddedSize"] =
      static_cast<double>(fetch::math::Tensor<float>::PaddedSizeFromShape(shape));

  for (auto _ : state)
  {
    state.PauseTiming();
    VMPtr vm;
    SetUp(vm);

    state.ResumeTiming();
    auto data = CreateTensor(vm, shape);
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

}  // namespace tensor
}  // namespace ml
}  // namespace benchmark
}  // namespace vm_modules
