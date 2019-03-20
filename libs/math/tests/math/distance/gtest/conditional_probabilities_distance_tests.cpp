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
#include <iomanip>
#include <iostream>

#include "math/distance/conditional_probabilities_distance.hpp"
#include "math/tensor.hpp"

using namespace fetch::math::distance;
using namespace fetch::math;

TEST(distance_tests, conditional_distance)
{
    Tensor<double> A = Tensor<double>({4, 4});

    A.Set({0,0},1);
    A.Set({0,1},2);
    A.Set({0,2},3);
    A.Set({0,3},4);

    A.Set({1,0},5);
    A.Set({1,1},6);
    A.Set({1,2},7);
    A.Set({1,3},8);

    A.Set({2,0},9);
    A.Set({2,1},10);
    A.Set({2,2},11);
    A.Set({2,3},12);

    A.Set({3,0},13);
    A.Set({3,1},14);
    A.Set({3,2},15);
    A.Set({3,3},16);

    std::cout<<ConditionalProbabilitiesDistance(A,1,2,0.5);
}
