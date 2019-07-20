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

#include "core/random/lcg.hpp"
#include "vectorise/memory/shared_array.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

namespace {

using namespace fetch::memory;

using data_type  = uint64_t;
using array_type = SharedArray<data_type>;

class TestClass : public ::testing::TestWithParam<int>
{
};

TEST_P(TestClass, basic_Test)
{
  static fetch::random::LinearCongruentialGenerator lcg1, lcg2;
  lcg1.Reset();
  lcg2.Reset();
  std::size_t N = lcg1() % 100000;
  lcg2();

  array_type array(N), other;
  for (std::size_t i = 0; i < N; ++i)
  {
    array[i] = lcg1();
  }

  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(array[i], lcg2()) << "1: memory doesn't store what it is supposed to";
  }

  other = array;
  lcg2.Reset();
  lcg2();
  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(other[i], lcg2()) << "2: memory doesn't store what it is supposed to";
  }

  array_type yao(other);
  lcg2.Reset();
  lcg2();
  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(yao[i], lcg2()) << "3: memory doesn't store what it is supposed to";
  }

  //  array = array;
  /*
  if(array.reference_count() != 3)
  {
    std::cout << "expected array to be referenced exactly 3 times";
    std::cout << "but is referenced " << array.reference_count() <<  std::endl;
  }
  if(testing::total_shared_objects != 1)
  {
    std::cout << "expected exactly 1 object but " <<
      testing::total_shared_objects;
    std::cout << "found" << std::endl;
  }
  */
  lcg1.Seed(lcg1());
  lcg2.Seed(lcg2());
}

INSTANTIATE_TEST_CASE_P(Basic_test, TestClass, ::testing::Range(0, 100), );

}  // namespace
