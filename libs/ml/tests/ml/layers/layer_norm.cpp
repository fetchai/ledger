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
#include "math/fundamental_operators.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class LayerNormTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(LayerNormTest, MyTypes);

TYPED_TEST(LayerNormTest, broadcast_test)
{
	using ArrayType = typename fetch::math::Tensor<float>;
	using DataType = float;
	
	auto a = ArrayType({3, 4});
	a.Fill(DataType(2));
	auto b = ArrayType({3, 1});
	b.Fill(DataType(3));
	auto c = ArrayType({3});
	c.Fill(DataType(3));
	
	ArrayType d;
	fetch::math::implementations::Multiply(a, c, d);
	
	std::cout << "d.ToString(): " << d.ToString() << std::endl;
}