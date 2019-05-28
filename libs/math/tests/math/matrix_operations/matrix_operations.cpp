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
  TypeParam array1{4};
  TypeParam mask{4};
  mask.SetAllZero();
  auto ret = fetch::math::BooleanMask(array1, mask);
  EXPECT_EQ(ret.size(), 0);

  mask.SetAllOne();
  ret = fetch::math::BooleanMask(array1, mask);
  EXPECT_EQ(ret.size(), array1.size());
  EXPECT_EQ(ret.shape(), array1.shape());
}

TYPED_TEST(FreeFunctionsTest, Scatter1D_SetAll)
{
  using DataType = typename TypeParam::Type;

  TypeParam               array1{4};
  TypeParam               updates{4};
  std::vector<SizeVector> indices{};
  updates.SetAllOne();
  indices.emplace_back(SizeVector{0});
  indices.emplace_back(SizeVector{1});
  indices.emplace_back(SizeVector{2});
  indices.emplace_back(SizeVector{3});

  for (std::size_t j{0}; j < array1.size(); ++j)
  {
    EXPECT_EQ(array1(j), DataType{0});
  }
  fetch::math::Scatter(array1, updates, indices);
  for (std::size_t j{0}; j < array1.size(); ++j)
  {
    EXPECT_EQ(array1(j), DataType{1});
  }
}

TYPED_TEST(FreeFunctionsTest, Scatter2D_SetAll)
{
  using DataType = typename TypeParam::Type;

  TypeParam array1{{4, 4}};
  TypeParam updates{16};
  updates.SetAllOne();
  std::vector<SizeVector> indices{};
  for (std::size_t j{0}; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k{0}; k < array1.shape()[1]; ++k)
    {
      indices.emplace_back(SizeVector{j, k});
    }
  }

  for (std::size_t j{0}; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k{0}; k < array1.shape()[1]; ++k)
    {
      EXPECT_EQ(array1(j, k), DataType{0});
    }
  }
  fetch::math::Scatter(array1, updates, indices);
  for (std::size_t j{0}; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k{0}; k < array1.shape()[1]; ++k)
    {
      EXPECT_EQ(array1(j, k), DataType{1});
    }
  }
}

TYPED_TEST(FreeFunctionsTest, Scatter3D_SetAll)
{
  using DataType = typename TypeParam::Type;

  TypeParam array1{{4, 4, 4}};
  TypeParam updates{64};
  updates.SetAllOne();
  std::vector<SizeVector> indices{};
  for (std::size_t j{0}; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k{0}; k < array1.shape()[1]; ++k)
    {
      for (std::size_t m{0}; m < array1.shape()[2]; ++m)
      {
        indices.emplace_back(SizeVector{j, k, m});
      }
    }
  }

  for (std::size_t j{0}; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k{0}; k < array1.shape()[1]; ++k)
    {
      for (std::size_t m{0}; m < array1.shape()[2]; ++m)
      {
        EXPECT_EQ(array1(j, k, m), DataType{0});
      }
    }
  }
  fetch::math::Scatter(array1, updates, indices);

  for (std::size_t j{0}; j < array1.shape()[0]; ++j)
  {
    for (std::size_t k{0}; k < array1.shape()[1]; ++k)
    {
      for (std::size_t m{0}; m < array1.shape()[2]; ++m)
      {
        EXPECT_EQ(array1(j, k, m), DataType{1});
      }
    }
  }
}

TYPED_TEST(FreeFunctionsTest, Product_OneDimension)
{
  using DataType = typename TypeParam::Type;
  TypeParam array1{4};

  array1(0) = DataType{0.3f};
  array1(1) = DataType{1.2f};
  array1(2) = DataType{0.7f};
  array1(3) = DataType{22};

  DataType output = fetch::math::Product(array1);
  EXPECT_NEAR(static_cast<double>(output), 5.544, 1e-6);

  array1(3) = 1;
  fetch::math::Product(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 0.252, 1e-6);

  array1(1) = 0;
  fetch::math::Product(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 0.0, 1e-6);
}

TYPED_TEST(FreeFunctionsTest, Product_TwoDimension)
{
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(1, 0) = DataType{1};
  array1(2, 0) = DataType{13};
  array1(3, 0) = DataType{21};

  array1(0, 1) = DataType{21};
  array1(1, 1) = DataType{1};
  array1(2, 1) = DataType{10};
  array1(3, 1) = DataType{-0.5};

  DataType output = fetch::math::Product(array1);
  EXPECT_NEAR(static_cast<double>(output), 487305.0, 1e-6);

  array1(1, 1) = 0;
  output       = fetch::math::Product(array1);
  EXPECT_NEAR(static_cast<double>(output), 0, 1e-6);
}

TYPED_TEST(FreeFunctionsTest, Max_OneDimension)
{
  using DataType = typename TypeParam::Type;

  TypeParam array1{4};

  array1(0) = 0.3f;
  array1(1) = 1.2f;
  array1(2) = 0.7f;
  array1(3) = 22;

  DataType output;
  fetch::math::Max(array1, output);
  EXPECT_EQ(output, array1(3));

  array1(3) = 0;
  fetch::math::Max(array1, output);
  EXPECT_EQ(output, array1(1));

  array1(1) = 0;
  fetch::math::Max(array1, output);
  EXPECT_EQ(output, array1(2));
}

TYPED_TEST(FreeFunctionsTest, Max_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{n_data};
  fetch::math::Max(array1, 1, output);
  EXPECT_EQ(output(0), DataType{21});
  EXPECT_EQ(output(1), DataType{0});
  EXPECT_EQ(output(2), DataType{999});
  EXPECT_EQ(output(3), DataType{21});

  TypeParam output2{n_features};
  fetch::math::Max(array1, 0, output2);
  EXPECT_EQ(output2(0), DataType{21});
  EXPECT_EQ(output2(1), DataType{999});
}

TYPED_TEST(FreeFunctionsTest, Min_OneDimension)
{
  using DataType = typename TypeParam::Type;

  TypeParam array1{4};

  array1(0) = DataType{0.3f};
  array1(1) = DataType{1.2f};
  array1(2) = DataType{0.7f};
  array1(3) = DataType{22};

  DataType output;
  fetch::math::Min(array1, output);
  EXPECT_EQ(output, array1(0));

  array1(0) = DataType{1000};
  fetch::math::Min(array1, output);
  EXPECT_EQ(output, array1(2));

  array1(2) = DataType{1000};
  fetch::math::Min(array1, output);
  EXPECT_EQ(output, array1(1));
}

TYPED_TEST(FreeFunctionsTest, Min_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{n_data};
  fetch::math::Min(array1, 1, output);
  EXPECT_EQ(output(0), DataType{-17});
  EXPECT_EQ(output(1), DataType{0});
  EXPECT_EQ(output(2), DataType{13});
  EXPECT_EQ(output(3), DataType{-0.5});

  TypeParam output2{n_features};
  fetch::math::Min(array1, 0, output2);
  EXPECT_EQ(output2(0), DataType{-17});
  EXPECT_EQ(output2(1), DataType{-0.5});
}

TYPED_TEST(FreeFunctionsTest, PeakToPeak_OneDimension)
{
  using DataType = typename TypeParam::Type;

  TypeParam array1{4};

  array1(0) = DataType{0.3f};
  array1(1) = DataType{1.2f};
  array1(2) = DataType{0.7f};
  array1(3) = DataType{22};

  DataType output;
  fetch::math::PeakToPeak(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 21.7, 1e-5);

  array1(3) = DataType{0.5};
  fetch::math::PeakToPeak(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 0.9, 1e-5);

  array1(1) = DataType{0.1f};
  fetch::math::PeakToPeak(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 0.6, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, PeakToPeak_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{n_data};
  fetch::math::PeakToPeak(array1, 1, output);
  EXPECT_NEAR(static_cast<double>(output(0)), 38, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1)), 0, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2)), 986, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(3)), 21.5, 1e-5);

  TypeParam output2{n_features};
  fetch::math::PeakToPeak(array1, 0, output2);
  EXPECT_NEAR(static_cast<double>(output2(0)), 38, 1e-5);
  EXPECT_NEAR(static_cast<double>(output2(1)), 999.5, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, Maximum_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};
  TypeParam array2{{n_data, n_features}};
  TypeParam output{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{-0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  array2(0, 0) = DataType{17};
  array2(0, 1) = DataType{-21};
  array2(1, 0) = DataType{0};
  array2(1, 1) = DataType{1};
  array2(2, 0) = DataType{3};
  array2(2, 1) = DataType{-999};
  array2(3, 0) = DataType{-0.1f};
  array2(3, 1) = DataType{0.5f};

  fetch::math::Maximum(array1, array2, output);
  EXPECT_EQ(output.shape().size(), 2);
  EXPECT_EQ(output.shape()[0], 4);
  EXPECT_EQ(output.shape()[1], 2);

  EXPECT_EQ(output(0, 0), DataType{17});
  EXPECT_EQ(output(0, 1), DataType{21});
  EXPECT_EQ(output(1, 0), DataType{-0});
  EXPECT_EQ(output(1, 1), DataType{1});
  EXPECT_EQ(output(2, 0), DataType{13});
  EXPECT_EQ(output(2, 1), DataType{999});
  EXPECT_EQ(output(3, 0), DataType{21});
  EXPECT_EQ(output(3, 1), DataType{0.5});
}

TYPED_TEST(FreeFunctionsTest, ArgMax_OneDimension)
{
  using DataType = typename TypeParam::Type;

  TypeParam array1{4};

  array1(0) = DataType{0.3f};
  array1(1) = DataType{1.2f};
  array1(2) = DataType{0.7f};
  array1(3) = DataType{22};

  TypeParam output{1};
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output(0), 3);

  array1(3) = 0;
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output(0), 1);

  array1(1) = 0;
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output(0), 2);
}

TYPED_TEST(FreeFunctionsTest, ArgMax_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{n_data};
  fetch::math::ArgMax(array1, output, 1);
  EXPECT_EQ(output(0), 1);
  EXPECT_EQ(output(1), 0);
  EXPECT_EQ(output(2), 1);
  EXPECT_EQ(output(3), 0);
}

TYPED_TEST(FreeFunctionsTest, ArgMax_TwoDimension_off_axis)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{n_features};
  fetch::math::ArgMax(array1, output, 0);
  EXPECT_EQ(output(0), 3);
  EXPECT_EQ(output(1), 2);
}

TYPED_TEST(FreeFunctionsTest, Sum_OneDimension)
{
  using DataType = typename TypeParam::Type;

  TypeParam array1{4};
  array1(0) = DataType{0.3f};
  array1(1) = DataType{1.2f};
  array1(2) = DataType{0.7f};
  array1(3) = DataType{22};

  DataType output;
  fetch::math::Sum(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 24.2, 1e-5);

  array1(3) = 0;
  fetch::math::Sum(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 2.2, 1e-5);

  array1(1) = 0;
  fetch::math::Sum(array1, output);
  EXPECT_NEAR(static_cast<double>(output), 1., 1e-5);
}

TYPED_TEST(FreeFunctionsTest, ReduceSum_axis_0)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{{1, n_features}};
  fetch::math::ReduceSum(array1, 0, output);

  EXPECT_NEAR(static_cast<double>(output(0, 0)), 17., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 1)), 1019.5, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, ReduceSum_axis_1)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{{n_data, 1}};
  fetch::math::ReduceSum(array1, 1, output);
  EXPECT_NEAR(static_cast<double>(output(0, 0)), 4., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 0)), 0., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 0)), 1012., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(3, 0)), 20.5, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, ReduceMean_axis_0)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{{1, n_features}};
  fetch::math::ReduceMean(array1, 0, output);

  EXPECT_NEAR(static_cast<double>(output(0, 0)), 8.5, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 1)), 509.75, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, ReduceMean_axis_1)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  SizeType  n_features{2};
  TypeParam array1{{n_data, n_features}};

  array1(0, 0) = DataType{-17};
  array1(0, 1) = DataType{21};
  array1(1, 0) = DataType{0};
  array1(1, 1) = DataType{0};
  array1(2, 0) = DataType{13};
  array1(2, 1) = DataType{999};
  array1(3, 0) = DataType{21};
  array1(3, 1) = DataType{-0.5};

  TypeParam output{{n_data, 1}};
  fetch::math::ReduceMean(array1, 1, output);
  EXPECT_NEAR(static_cast<double>(output(0, 0)), 1., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 0)), 0., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 0)), 253., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(3, 0)), 5.125, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, Dot)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType matrix_one_width{3};
  SizeType matrix_one_height{2};
  SizeType matrix_two_width{2};
  SizeType matrix_two_height{4};

  TypeParam array1{{matrix_one_width, matrix_one_height}};
  TypeParam array2{{matrix_two_width, matrix_two_height}};

  DataType cnt{0};
  auto     it = array1.begin();
  while (it.is_valid())
  {
    *it = cnt;
    ++it;
    cnt += DataType{1};
  }

  DataType cnt2{0};
  auto     it2 = array2.begin();
  while (it2.is_valid())
  {
    *it2 = cnt2;
    ++it2;
    cnt2 += DataType{1};
  }

  TypeParam output{{matrix_one_width, matrix_two_height}};
  fetch::math::Dot(array1, array2, output);

  EXPECT_NEAR(static_cast<double>(output(0, 0)), 3, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 1)), 9, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 2)), 15, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 3)), 21, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 0)), 4, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 1)), 14, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 2)), 24, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 3)), 34, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 0)), 5, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 1)), 19, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 2)), 33, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 3)), 47, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, DotTranspose)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType matrix_one_width{3};
  SizeType matrix_one_height{2};
  SizeType matrix_two_width{4};
  SizeType matrix_two_height{2};

  TypeParam array1{{matrix_one_width, matrix_one_height}};
  TypeParam array2{{matrix_two_width, matrix_two_height}};

  DataType cnt{0};
  auto     it = array1.begin();
  while (it.is_valid())
  {
    *it = cnt;
    ++it;
    cnt += DataType{1};
  }

  DataType cnt2{0};
  auto     it2 = array2.begin();
  while (it2.is_valid())
  {
    *it2 = cnt2;
    ++it2;
    cnt2 += DataType{1};
  }

  TypeParam output{{matrix_one_width, matrix_two_width}};
  fetch::math::DotTranspose(array1, array2, output);

  EXPECT_NEAR(static_cast<double>(output(0, 0)), 12, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 1)), 15, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 2)), 18, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 3)), 21, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 0)), 16, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 1)), 21, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 2)), 26, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 3)), 31, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 0)), 20, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 1)), 27, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 2)), 34, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 3)), 41, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, TransposeDot)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType matrix_one_width{2};
  SizeType matrix_one_height{3};
  SizeType matrix_two_width{2};
  SizeType matrix_two_height{4};

  TypeParam array1{{matrix_one_width, matrix_one_height}};
  TypeParam array2{{matrix_two_width, matrix_two_height}};

  DataType cnt{0};
  auto     it = array1.begin();
  while (it.is_valid())
  {
    *it = cnt;
    ++it;
    cnt += DataType{1};
  }

  DataType cnt2{0};
  auto     it2 = array2.begin();
  while (it2.is_valid())
  {
    *it2 = cnt2;
    ++it2;
    cnt2 += DataType{1};
  }

  TypeParam output{{matrix_one_height, matrix_two_height}};
  fetch::math::TransposeDot(array1, array2, output);

  EXPECT_NEAR(static_cast<double>(output(0, 0)), 1, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 1)), 3, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 2)), 5, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(0, 3)), 7, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 0)), 3, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 1)), 13, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 2)), 23, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1, 3)), 33, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 0)), 5, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 1)), 23, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 2)), 41, 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2, 3)), 59, 1e-5);
}

TYPED_TEST(FreeFunctionsTest, DynamicStitch)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{6};
  TypeParam data{{n_data}};
  TypeParam indicies{{n_data}};

  data(0) = DataType{-2};
  data(1) = DataType{3};
  data(2) = DataType{-4};
  data(3) = DataType{5};
  data(4) = DataType{-6};
  data(5) = DataType{7};

  indicies(0) = DataType{5};
  indicies(1) = DataType{4};
  indicies(2) = DataType{3};
  indicies(3) = DataType{2};
  indicies(4) = DataType{1};
  indicies(5) = DataType{0};

  TypeParam output{{n_data}};
  fetch::math::DynamicStitch(output, indicies, data);
  EXPECT_NEAR(static_cast<double>(output(0)), 7., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(1)), -6., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(2)), 5., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(3)), -4., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(4)), 3., 1e-5);
  EXPECT_NEAR(static_cast<double>(output(5)), -2., 1e-5);
}
