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

#include "math/base_types.hpp"
#include "math/top_k.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class TopKTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TopKTest, TensorFloatingTypes);

TYPED_TEST(TopKTest, top_k_test_sorted)
{
  using DataType    = typename TypeParam::Type;
  using SizeType    = typename TypeParam::SizeType;
  using ArrayType   = TypeParam;
  using IndicesType = fetch::math::Tensor<SizeType>;

  ArrayType data = TypeParam::FromString("1,4,3,2;5,6,7,8;9,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  ArrayType gt_data = TypeParam::FromString("4,3;8,7;12,11;16,15");
  gt_data.Reshape({4, 2});
  IndicesType gt_indices = IndicesType::FromString("1,2;3,2;3,2;3,2");
  gt_indices.Reshape({4, 2});

  SizeType                          k   = 2;
  std::pair<ArrayType, IndicesType> ret = TopK<ArrayType, IndicesType>(data, k, true);

  ASSERT_TRUE(ret.first.AllClose(gt_data, fetch::math::function_tolerance<DataType>(),
                                 fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(ret.second.AllClose(gt_indices, 0, 0));
}

TYPED_TEST(TopKTest, top_k_test_unsorted)
{
  using DataType    = typename TypeParam::Type;
  using SizeType    = typename TypeParam::SizeType;
  using ArrayType   = TypeParam;
  using IndicesType = fetch::math::Tensor<SizeType>;

  ArrayType data = TypeParam::FromString("1,4,3,2;5,6,7,8;9,10,11,12;13,14,15,16");
  data.Reshape({4, 4});
  ArrayType gt_data = TypeParam::FromString("3,4;7,8;11,12;15,16");
  gt_data.Reshape({4, 2});
  IndicesType gt_indices = IndicesType::FromString("2,1;2,3;2,3;2,3");
  gt_indices.Reshape({4, 2});

  SizeType                          k   = 2;
  std::pair<ArrayType, IndicesType> ret = TopK<ArrayType, IndicesType>(data, k, false);

  ASSERT_TRUE(ret.first.AllClose(gt_data, fetch::math::function_tolerance<DataType>(),
                                 fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(ret.second.AllClose(gt_indices, 0, 0));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
