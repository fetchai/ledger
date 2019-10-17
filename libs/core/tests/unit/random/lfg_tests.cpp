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

#include "bit_statistics.hpp"
#include "core/random/lfg.hpp"

#include "gtest/gtest.h"

#include <iostream>

TEST(lfg_gtest, basic_test)
{
  BitStatistics<> bst;

  ASSERT_TRUE(bst.TestAccuracy(1000000, 0.002));
  for (auto &a : bst.GetProbabilities())
  {
    std::cout << a << " ";
  }
  std::cout << std::endl;
}
