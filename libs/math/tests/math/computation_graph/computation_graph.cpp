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

#include <iomanip>
#include <iostream>
#include <gtest/gtest.h>

#include "math/computation_graph/computation_graph.hpp"

//using namespace fetch::math;

using namespace fetch::math::computation_graph;
TEST(computation_graph, arithmetic_parse)
{

//  ComputationGraph<double> computation_graph;
  ComputationGraph computation_graph;
  computation_graph.ParseExpression("2 + 1 - (3 * 4)");

  computation_graph.Run();

  //  ASSERT_TRUE(double(result) == double(-9));
}
