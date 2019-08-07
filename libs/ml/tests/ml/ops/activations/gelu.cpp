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
#include "ml/ops/activation.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <vector>

template <typename T>
class GeluTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(GeluTest, MyTypes);

TYPED_TEST(GeluTest, forward_test_3d) {
	
	using ArrayType = TypeParam;
	using DataType = typename ArrayType::Type;
	
	ArrayType data = ArrayType::FromString("-10, -2, -1, -0.5, 0, 0.2, 1.6, 5.7, 12");
	data.Reshape({3, 1, 3});
	ArrayType gt   = ArrayType::FromString("-0.0000000000, -0.0454022884, -0.1588079929, -0.1542859972, 0.0000000000,  0.1158514246,  1.5121370554,  5.6999998093, 12.0000000000");
	gt.Reshape({3, 1, 3});
	
	fetch::ml::ops::Gelu<ArrayType> op;
	ArrayType prediction(op.ComputeOutputShape({std::make_shared<const ArrayType>(data)}));
	op.Forward({std::make_shared<const ArrayType>(data)}, prediction);
	// test correct values
	ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(), static_cast<DataType>(2.8) * fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GeluTest, backward_3d_test)
{
	using ArrayType = TypeParam;
	using DataType = typename ArrayType::Type;
	
	ArrayType data = ArrayType::FromString("-10, -2, -1, -0.5, 0, 0.2, 1.6, 5.7, 12");
//	data.Reshape({3, 1, 3});
	ArrayType error_signal  = ArrayType::FromString("-3, 2, 3, 4.5, 0.2, 6.6, 7.1, 10, 0.02");
//	error_signal.Reshape({3, 1, 3});
	ArrayType gt   = ArrayType::FromString("0.0000000000, -0.1721984446, -0.2488922477,  0.5968354940, 0.1000000015,  4.3392238617,  7.9740133286, 10.0000000000, 0.0199999996");
//	gt.Reshape({3, 1, 3});
	
	fetch::ml::ops::Gelu<ArrayType> op;
	std::vector<ArrayType> prediction = op.Backward({std::make_shared<const ArrayType>(data)}, error_signal);
	
	std::cout << "prediction[0].ToString(): " << prediction[0].ToString() << std::endl;
	// test correct values
	ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(), static_cast<DataType>(5) * fetch::math::function_tolerance<DataType>()));
}