//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <stack>

#include "core/random/lfg.hpp"
#include "storage/variant_stack.hpp"

using fetch::storage::VariantStack;

class TestClass
{
public:
  uint64_t value1 = 0;
  uint8_t  value2 = 0;

  bool operator==(TestClass const &rhs)
  {
    return value1 == rhs.value1 && value2 == rhs.value2;
  }
};
class VariantStackBench : public ::benchmark::Fixture
{
protected:
  void SetUp(const ::benchmark::State &st) override
  {
    stack_.New("Variant_bench.db");
  }

  void TearDown(const ::benchmark::State &) override
  {}

  VariantStack                              stack_;
  fetch::random::LaggedFibonacciGenerator<> lfg_;
};

BENCHMARK_F(VariantStackBench, WritingIntToStack)(benchmark::State &st)
{
   TestClass temp;
   uint64_t choose_type;
   uint64_t i=0;
  for (auto _ : st)
  {
    uint64_t random = lfg_();
    temp.value1 = random;
    temp.value2 = random & 0xFF;
    std::tuple<TestClass, uint64_t, uint8_t> test_vals =
        std::make_tuple(temp, uint64_t(~random), uint8_t(~random & 0xFF));
     choose_type = i % 3;
     ++i;
     stack_.Push(std::get<0>(test_vals), choose_type);
  }
}