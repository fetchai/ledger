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

using namespace fetch::math::computation_graph;

TEST(computation_graph, simple_arithmetic)
{
  double result;
  ComputationGraph computation_graph;

  computation_graph.ParseExpression("1 + 2");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(3));

  computation_graph.Reset();
  computation_graph.ParseExpression("1 - 2");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(-1));

  computation_graph.Reset();
  computation_graph.ParseExpression("1 * 2");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(2));

  computation_graph.Reset();
  computation_graph.ParseExpression("1 / 2");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(0.5));

}

TEST(computation_graph, multi_parenthesis_test)
{
  double result;
  ComputationGraph computation_graph;

  computation_graph.ParseExpression("(((1 + 2)))");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(3));

  computation_graph.Reset();
  computation_graph.ParseExpression("((1 - 2) * (3 / 4))");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(-0.75));

}

TEST(computation_graph, odd_num_nodes)
{
  double result;
  ComputationGraph computation_graph;

  computation_graph.ParseExpression("4 * 6 / 3");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(8));
}

TEST(computation_graph, multi_digit_nums)
{
  double result;
  ComputationGraph computation_graph;

  computation_graph.ParseExpression("100 * 62 / 31");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(200));
}

TEST(computation_graph, decimal_place_nums)
{
  double result;
  ComputationGraph computation_graph;

  computation_graph.ParseExpression("10.0 * 62.5 / 31.25");
  computation_graph.Run(result);
  std::cout << "result: " << result << std::endl;
  ASSERT_TRUE(result == double(20));
}
