//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "ml/utilities/sparse_tensor_utilities.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include <vector>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class SparseTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SparseTest, math::test::TensorFloatingTypes);
TYPED_TEST(SparseTest, from_sparse_test)
{
  using SizeType = fetch::math::SizeType;
  using SizeSet  = std::unordered_set<SizeType>;

  std::vector<SizeType> tensor_shape = {3, 3};

  TypeParam sparse_data(tensor_shape);
  sparse_data.FillUniformRandom();

  SizeSet rows({1, 2, 4});

  TypeParam data = utilities::FromSparse(sparse_data, rows, 5);

  std::vector<SizeType> gt_shape = {3, 5};
  EXPECT_EQ(data.shape(), gt_shape);

  EXPECT_TRUE(data.View(4).Copy().AllClose(sparse_data.View(0).Copy()));
  EXPECT_TRUE(data.View(3).Copy().AllClose(TypeParam({3, 1})));
  EXPECT_TRUE(data.View(2).Copy().AllClose(sparse_data.View(1).Copy()));
  EXPECT_TRUE(data.View(1).Copy().AllClose(sparse_data.View(2).Copy()));
  EXPECT_TRUE(data.View(0).Copy().AllClose(TypeParam({3, 1})));
}

TYPED_TEST(SparseTest, to_sparse_test)
{
  using SizeType = fetch::math::SizeType;
  using SizeSet  = std::unordered_set<SizeType>;

  std::vector<SizeType> tensor_shape = {3, 5};

  TypeParam data(tensor_shape);
  data.FillUniformRandom();

  SizeSet rows({1, 2, 4});

  TypeParam sparse_data = utilities::ToSparse(data, rows);

  std::vector<SizeType> gt_shape = {3, 3};
  EXPECT_EQ(sparse_data.shape(), gt_shape);

  EXPECT_TRUE(sparse_data.View(0).Copy().AllClose(data.View(4).Copy()));
  EXPECT_TRUE(sparse_data.View(1).Copy().AllClose(data.View(2).Copy()));
  EXPECT_TRUE(sparse_data.View(2).Copy().AllClose(data.View(1).Copy()));
}

TYPED_TEST(SparseTest, sparse_add_sparse_to_normal_test)
{
  using DataType = typename TypeParam::Type;
  using SizeType = fetch::math::SizeType;
  using SizeSet  = std::unordered_set<SizeType>;

  std::vector<SizeType> tensor_shape_src = {3, 3};
  TypeParam             data_src(tensor_shape_src);
  data_src.FillUniformRandom();

  std::vector<SizeType> tensor_shape_dst = {3, 5};
  TypeParam             data_dst(tensor_shape_dst);
  data_dst.FillUniformRandom();

  TypeParam data_dst_old = data_dst.Copy();

  SizeSet rows({1, 2, 4});

  utilities::SparseAdd(data_src, data_dst, rows);

  EXPECT_TRUE(data_dst.View(4).Copy().AllClose(
      data_dst_old.View(4).Copy() + data_src.View(0).Copy(),
      fetch::math::function_tolerance<DataType>(), fetch::math::function_tolerance<DataType>()));
  EXPECT_TRUE(data_dst.View(3).Copy().AllClose(data_dst_old.View(3).Copy(),
                                               fetch::math::function_tolerance<DataType>(),
                                               fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(data_dst.View(2).Copy().AllClose(
      data_dst_old.View(2).Copy() + data_src.View(1).Copy(),
      fetch::math::function_tolerance<DataType>(), fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(data_dst.View(1).Copy().AllClose(
      data_dst_old.View(1).Copy() + data_src.View(2).Copy(),
      fetch::math::function_tolerance<DataType>(), fetch::math::function_tolerance<DataType>()));
  EXPECT_TRUE(data_dst.View(0).Copy().AllClose(data_dst_old.View(0).Copy(),
                                               fetch::math::function_tolerance<DataType>(),
                                               fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SparseTest, sparse_vector_add_sparse_to_normal_test)
{
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;
  using SizeVector = std::vector<SizeType>;

  std::vector<SizeType> tensor_shape_src = {3, 3};
  TypeParam             data_src(tensor_shape_src);
  data_src.FillUniformRandom();

  std::vector<SizeType> tensor_shape_dst = {3, 5};
  TypeParam             data_dst(tensor_shape_dst);
  data_dst.FillUniformRandom();

  TypeParam data_dst_old = data_dst.Copy();

  SizeVector rows({1, 2, 4});

  utilities::SparseAdd(data_src, data_dst, rows);

  EXPECT_TRUE(data_dst.View(1).Copy().AllClose(
      data_dst_old.View(1).Copy() + data_src.View(0).Copy(),
      fetch::math::function_tolerance<DataType>(), fetch::math::function_tolerance<DataType>()));
  EXPECT_TRUE(data_dst.View(3).Copy().AllClose(data_dst_old.View(3).Copy(),
                                               fetch::math::function_tolerance<DataType>(),
                                               fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(data_dst.View(2).Copy().AllClose(
      data_dst_old.View(2).Copy() + data_src.View(1).Copy(),
      fetch::math::function_tolerance<DataType>(), fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(data_dst.View(4).Copy().AllClose(
      data_dst_old.View(4).Copy() + data_src.View(2).Copy(),
      fetch::math::function_tolerance<DataType>(), fetch::math::function_tolerance<DataType>()));
  EXPECT_TRUE(data_dst.View(0).Copy().AllClose(data_dst_old.View(0).Copy(),
                                               fetch::math::function_tolerance<DataType>(),
                                               fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
