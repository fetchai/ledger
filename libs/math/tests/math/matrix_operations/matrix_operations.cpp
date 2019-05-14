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

#include "core/random/lcg.hpp"
#include "math/fixed_point/fixed_point.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"

#include <chrono>

using namespace fetch::math;

template <typename T>
class FreeFunctionsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(FreeFunctionsTest, MyTypes);

TYPED_TEST(FreeFunctionsTest, BooleanMask_SetAll)
{
  using SizeType = typename TypeParam::SizeType;

  TypeParam array1{4};
  TypeParam mask{4};
  mask.SetAllZero();
  auto ret = fetch::math::BooleanMask(array1, mask);
  EXPECT_EQ(ret.size(), SizeType(0));

  mask.SetAllOne();
  ret = fetch::math::BooleanMask(array1, mask);
  EXPECT_EQ(ret.size(), array1.size());
  EXPECT_EQ(ret.shape(), array1.shape());
}

TYPED_TEST(FreeFunctionsTest, Scatter1D_SetAll)
{
  using Type = typename TypeParam::Type;

  TypeParam               array1{4};
  TypeParam               updates{4};
  std::vector<SizeVector> indices{};
  updates.SetAllOne();
  indices.emplace_back(SizeVector{0});
  indices.emplace_back(SizeVector{1});
  indices.emplace_back(SizeVector{2});
  indices.emplace_back(SizeVector{3});

  for (std::size_t j = 0; j < array1.size(); ++j)
  {
    EXPECT_EQ(array1.At(j), Type(0));
  }
  fetch::math::Scatter(array1, updates, indices);
  for (std::size_t j = 0; j < array1.size(); ++j)
  {
    EXPECT_EQ(array1.At(j), Type(1));
  }
}

TYPED_TEST(FreeFunctionsTest, Scatter2D_SetAll)
{
  using Type = typename TypeParam::Type;

  TypeParam array1{{4, 4}};
  TypeParam updates{16};
  updates.SetAllOne();
  std::vector<SizeVector> indices{};
  for (std::size_t j = 0; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k = 0; k < array1.shape()[1]; ++k)
    {
      indices.emplace_back(SizeVector{j, k});
    }
  }

  for (std::size_t j = 0; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k = 0; k < array1.shape()[1]; ++k)
    {
      EXPECT_EQ(array1.At(j, k), Type(0));
    }
  }
  fetch::math::Scatter(array1, updates, indices);
  for (std::size_t j = 0; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k = 0; k < array1.shape()[1]; ++k)
    {
      EXPECT_EQ(array1.At(j, k), Type(1));
    }
  }
}

TYPED_TEST(FreeFunctionsTest, Scatter3D_SetAll)
{
  using Type = typename TypeParam::Type;

  TypeParam array1{{4, 4, 4}};
  TypeParam updates{64};
  updates.SetAllOne();
  std::vector<SizeVector> indices{};
  for (std::size_t j = 0; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k = 0; k < array1.shape()[1]; ++k)
    {
      for (std::size_t m = 0; m < array1.shape()[2]; ++m)
      {
        indices.emplace_back(SizeVector{j, k, m});
      }
    }
  }

  for (std::size_t j = 0; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k = 0; k < array1.shape()[1]; ++k)
    {
      for (std::size_t m = 0; m < array1.shape()[2]; ++m)
      {
        EXPECT_EQ(array1.At(j, k, m), Type(0));
      }
    }
  }
  fetch::math::Scatter(array1, updates, indices);

  for (std::size_t j = 0; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k = 0; k < array1.shape()[1]; ++k)
    {
      for (std::size_t m = 0; m < array1.shape()[2]; ++m)
      {
        EXPECT_EQ(array1.At(j, k, m), Type(1));
      }
    }
  }
}

TYPED_TEST(FreeFunctionsTest, Product_OneDimension)
{
  using SizeType = typename TypeParam::SizeType;
  using Type     = typename TypeParam::Type;
  TypeParam array1{4};

  array1.Set(SizeType{0}, typename TypeParam::Type(0.3));
  array1.Set(SizeType{1}, typename TypeParam::Type(1.2));
  array1.Set(SizeType{2}, typename TypeParam::Type(0.7));
  array1.Set(SizeType{3}, typename TypeParam::Type(22));

  Type output = fetch::math::Product(array1);
  EXPECT_NEAR(double(output), 5.544, 1e-6);

  array1.Set(3, typename TypeParam::Type(1));
  fetch::math::Product(array1, output);
  EXPECT_NEAR(double(output), 0.252, 1e-6);

  array1.Set(1, typename TypeParam::Type(0));
  fetch::math::Product(array1, output);
  EXPECT_NEAR(double(output), 0.0, 1e-6);
}

TYPED_TEST(FreeFunctionsTest, Product_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;
  using Type     = typename TypeParam::Type;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};

  array1.Set(0, 0, typename TypeParam::Type(-17));
  array1.Set(0, 1, typename TypeParam::Type(21));
  array1.Set(1, 0, typename TypeParam::Type(1));
  array1.Set(1, 1, typename TypeParam::Type(1));
  array1.Set(2, 0, typename TypeParam::Type(13));
  array1.Set(2, 1, typename TypeParam::Type(10));
  array1.Set(3, 0, typename TypeParam::Type(21));
  array1.Set(3, 1, typename TypeParam::Type(-0.5));

  Type output = fetch::math::Product(array1);
  EXPECT_NEAR(double(output), 487305.0, 1e-6);

  array1.Set(1, 1, typename TypeParam::Type(0));
  output = fetch::math::Product(array1);
  EXPECT_NEAR(double(output), 0, 1e-6);
}

TYPED_TEST(FreeFunctionsTest, Max_OneDimension)
{
  TypeParam array1{4};

  array1.Set(0, typename TypeParam::Type(0.3));
  array1.Set(1, typename TypeParam::Type(1.2));
  array1.Set(2, typename TypeParam::Type(0.7));
  array1.Set(3, typename TypeParam::Type(22));

  typename TypeParam::Type output;
  fetch::math::Max(array1, output);
  EXPECT_EQ(output, array1.At(3));

  array1.Set(3, typename TypeParam::Type(0));
  fetch::math::Max(array1, output);
  EXPECT_EQ(output, array1.At(1));

  array1.Set(1, typename TypeParam::Type(0));
  fetch::math::Max(array1, output);
  EXPECT_EQ(output, array1.At(2));
}

TYPED_TEST(FreeFunctionsTest, Max_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};

  array1.Set(0, 0, typename TypeParam::Type(-17));
  array1.Set(0, 1, typename TypeParam::Type(21));
  array1.Set(1, 0, typename TypeParam::Type(0));
  array1.Set(1, 1, typename TypeParam::Type(0));
  array1.Set(2, 0, typename TypeParam::Type(13));
  array1.Set(2, 1, typename TypeParam::Type(999));
  array1.Set(3, 0, typename TypeParam::Type(21));
  array1.Set(3, 1, typename TypeParam::Type(-0.5));

  TypeParam output{n_data};
  fetch::math::Max(array1, SizeType(1), output);
  EXPECT_EQ(output.At(0), typename TypeParam::Type(21));
  EXPECT_EQ(output.At(1), typename TypeParam::Type(0));
  EXPECT_EQ(output.At(2), typename TypeParam::Type(999));
  EXPECT_EQ(output.At(3), typename TypeParam::Type(21));

  TypeParam output2{n_features};
  fetch::math::Max(array1, SizeType(0), output2);
  EXPECT_EQ(output2.At(0), typename TypeParam::Type(21));
  EXPECT_EQ(output2.At(1), typename TypeParam::Type(999));
}

TYPED_TEST(FreeFunctionsTest, Min_OneDimension)
{
  TypeParam array1{4};

  array1.Set(0, typename TypeParam::Type(0.3));
  array1.Set(1, typename TypeParam::Type(1.2));
  array1.Set(2, typename TypeParam::Type(0.7));
  array1.Set(3, typename TypeParam::Type(22));

  typename TypeParam::Type output;
  fetch::math::Min(array1, output);
  EXPECT_EQ(output, array1.At(0));

  array1.Set(0, typename TypeParam::Type(1000));
  fetch::math::Min(array1, output);
  EXPECT_EQ(output, array1.At(2));

  array1.Set(2, typename TypeParam::Type(1000));
  fetch::math::Min(array1, output);
  EXPECT_EQ(output, array1.At(1));
}

TYPED_TEST(FreeFunctionsTest, Min_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};

  array1.Set(0, 0, typename TypeParam::Type(-17));
  array1.Set(0, 1, typename TypeParam::Type(21));
  array1.Set(1, 0, typename TypeParam::Type(0));
  array1.Set(1, 1, typename TypeParam::Type(0));
  array1.Set(2, 0, typename TypeParam::Type(13));
  array1.Set(2, 1, typename TypeParam::Type(999));
  array1.Set(3, 0, typename TypeParam::Type(21));
  array1.Set(3, 1, typename TypeParam::Type(-0.5));

  TypeParam output{n_data};
  fetch::math::Min(array1, SizeType(1), output);
  EXPECT_EQ(output.At(0), typename TypeParam::Type(-17));
  EXPECT_EQ(output.At(1), typename TypeParam::Type(0));
  EXPECT_EQ(output.At(2), typename TypeParam::Type(13));
  EXPECT_EQ(output.At(3), typename TypeParam::Type(-0.5));

  TypeParam output2{n_features};
  fetch::math::Min(array1, SizeType(0), output2);
  EXPECT_EQ(output2.At(0), typename TypeParam::Type(-17));
  EXPECT_EQ(output2.At(1), typename TypeParam::Type(-0.5));
}

TYPED_TEST(FreeFunctionsTest, Maximum_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};
  TypeParam array2{{n_data, n_features}};
  TypeParam output{{n_data, n_features}};

  array1.Set(0, 0, typename TypeParam::Type(-17));
  array1.Set(0, 1, typename TypeParam::Type(21));
  array1.Set(1, 0, typename TypeParam::Type(-0));
  array1.Set(1, 1, typename TypeParam::Type(0));
  array1.Set(2, 0, typename TypeParam::Type(13));
  array1.Set(2, 1, typename TypeParam::Type(999));
  array1.Set(3, 0, typename TypeParam::Type(21));
  array1.Set(3, 1, typename TypeParam::Type(-0.5));

  array2.Set(0, 0, typename TypeParam::Type(17));
  array2.Set(0, 1, typename TypeParam::Type(-21));
  array2.Set(1, 0, typename TypeParam::Type(0));
  array2.Set(1, 1, typename TypeParam::Type(1));
  array2.Set(2, 0, typename TypeParam::Type(3));
  array2.Set(2, 1, typename TypeParam::Type(-999));
  array2.Set(3, 0, typename TypeParam::Type(-0.1));
  array2.Set(3, 1, typename TypeParam::Type(0.5));

  fetch::math::Maximum(array1, array2, output);
  EXPECT_EQ(output.shape().size(), SizeType(2));
  EXPECT_EQ(output.shape()[0], SizeType(4));
  EXPECT_EQ(output.shape()[1], SizeType(2));

  EXPECT_EQ(output.At(0, 0), typename TypeParam::Type(17));
  EXPECT_EQ(output.At(0, 1), typename TypeParam::Type(21));
  EXPECT_EQ(output.At(1, 0), typename TypeParam::Type(-0));
  EXPECT_EQ(output.At(1, 1), typename TypeParam::Type(1));
  EXPECT_EQ(output.At(2, 0), typename TypeParam::Type(13));
  EXPECT_EQ(output.At(2, 1), typename TypeParam::Type(999));
  EXPECT_EQ(output.At(3, 0), typename TypeParam::Type(21));
  EXPECT_EQ(output.At(3, 1), typename TypeParam::Type(0.5));
}

TYPED_TEST(FreeFunctionsTest, ArgMax_OneDimension)
{
  using SizeType = typename TypeParam::SizeType;

  TypeParam array1{4};

  array1.Set(SizeType{0}, typename TypeParam::Type(0.3));
  array1.Set(SizeType{1}, typename TypeParam::Type(1.2));
  array1.Set(SizeType{2}, typename TypeParam::Type(0.7));
  array1.Set(SizeType{3}, typename TypeParam::Type(22));

  TypeParam output{1};
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(3));

  array1.Set(SizeType{3}, typename TypeParam::Type(0));
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(1));

  array1.Set(SizeType{1}, typename TypeParam::Type(0));
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(2));
}

TYPED_TEST(FreeFunctionsTest, ArgMax_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};

  array1.Set(SizeType{0}, SizeType{0}, typename TypeParam::Type(-17));
  array1.Set(SizeType{0}, SizeType{1}, typename TypeParam::Type(21));
  array1.Set(SizeType{1}, SizeType{0}, typename TypeParam::Type(0));
  array1.Set(SizeType{1}, SizeType{1}, typename TypeParam::Type(0));
  array1.Set(SizeType{2}, SizeType{0}, typename TypeParam::Type(13));
  array1.Set(SizeType{2}, SizeType{1}, typename TypeParam::Type(999));
  array1.Set(SizeType{3}, SizeType{0}, typename TypeParam::Type(21));
  array1.Set(SizeType{3}, SizeType{1}, typename TypeParam::Type(-0.5));

  TypeParam output{n_data};
  fetch::math::ArgMax(array1, output, SizeType(1));
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(1));
  EXPECT_EQ(output.At(1), typename TypeParam::SizeType(0));
  EXPECT_EQ(output.At(2), typename TypeParam::SizeType(1));
  EXPECT_EQ(output.At(3), typename TypeParam::SizeType(0));
}

TYPED_TEST(FreeFunctionsTest, ArgMax_TwoDimension_off_axis)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};

  array1.Set(SizeType{0}, SizeType{0}, typename TypeParam::Type(-17));
  array1.Set(SizeType{0}, SizeType{1}, typename TypeParam::Type(21));
  array1.Set(SizeType{1}, SizeType{0}, typename TypeParam::Type(0));
  array1.Set(SizeType{1}, SizeType{1}, typename TypeParam::Type(0));
  array1.Set(SizeType{2}, SizeType{0}, typename TypeParam::Type(13));
  array1.Set(SizeType{2}, SizeType{1}, typename TypeParam::Type(999));
  array1.Set(SizeType{3}, SizeType{0}, typename TypeParam::Type(21));
  array1.Set(SizeType{3}, SizeType{1}, typename TypeParam::Type(-0.5));

  TypeParam output{n_features};
  fetch::math::ArgMax(array1, output, SizeType(0));
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(3));
  EXPECT_EQ(output.At(1), typename TypeParam::SizeType(2));
}