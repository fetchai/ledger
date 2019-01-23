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

#include <gtest/gtest.h>
#include <iostream>

#include "core/random/lfg.hpp"
#include "math/rectangular_array.hpp"

using namespace fetch::math;
static fetch::random::LinearCongruentialGenerator gen;

TEST(rectangular_load_save_gtest, basic_tests)
{
  RectangularArray<uint64_t> a, b;
  a.Resize(3, 3);
  for (auto &v : a)
  {
    v = gen();
  }
  std::cout << "Saving " << std::endl;

  a.Save("test.array");
  std::cout << "Loading" << std::endl;
  b.Load("test.array");
  std::cout << "Ready" << std::endl;
  ASSERT_EQ(a.size(), b.size());
  std::cout << "Checking " << std::endl;

  for (std::size_t i = 0; i < a.size(); ++i)
  {
    ASSERT_EQ(a[i], b[i]) << "Failed 1: " << a.size() << " " << b.size();
  }
}
