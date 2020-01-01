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
#include "math/tensor/tensor.hpp"
#include "math/trigonometry.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class TrigTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TrigTest, HighPrecisionFloatingTypes);

TYPED_TEST(TrigTest, sin)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::Sin(val);
  EXPECT_NEAR(double(ret), double(0.29552022),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("1.2");
  ret = fetch::math::Sin(val);
  EXPECT_NEAR(double(ret), double(0.93203908),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::Sin(val);
  EXPECT_NEAR(double(ret), double(0.64421767),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("22");
  ret = fetch::math::Sin(val);
  EXPECT_NEAR(double(ret), double(-0.0088513093),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, sin_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("1.2"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("22"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::Sin(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.29552022"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("0.93203908"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.64421767"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-0.0088513093"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, cos)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::Cos(val);
  EXPECT_NEAR(double(ret), double(0.95533651),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("1.2");
  ret = fetch::math::Cos(val);
  EXPECT_NEAR(double(ret), double(0.36235771),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::Cos(val);
  EXPECT_NEAR(double(ret), double(0.76484221),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("22");
  ret = fetch::math::Cos(val);
  EXPECT_NEAR(double(ret), double(-0.99996084),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, cos_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("1.2"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("22"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::Cos(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.95533651"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("0.36235771"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.76484221"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-0.99996084"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, tan)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::Tan(val);
  EXPECT_NEAR(double(ret), double(0.30933625),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("1.2");
  ret = fetch::math::Tan(val);
  EXPECT_NEAR(double(ret), double(2.5721519),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::Tan(val);
  EXPECT_NEAR(double(ret), double(0.84228837),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("22");
  ret = fetch::math::Tan(val);
  EXPECT_NEAR(double(ret), double(0.0088516558),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, tan_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("1.2"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("22"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::Tan(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.30933625"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("2.5721519"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.84228837"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("0.0088516558"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, asin)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::ASin(val);
  EXPECT_NEAR(double(ret), double(0.30469266),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::ASin(val);
  EXPECT_NEAR(double(ret), double(-0.10016742),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::ASin(val);
  EXPECT_NEAR(double(ret), double(0.77539748),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::ASin(val);
  EXPECT_NEAR(double(ret), double(-1.1197695),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, asin_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ASin(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.30469266"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("-0.10016742"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.77539748"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-1.1197695"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, acos)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::ACos(val);
  EXPECT_NEAR(double(ret), double(1.2661037),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::ACos(val);
  EXPECT_NEAR(double(ret), double(1.6709638),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::ACos(val);
  EXPECT_NEAR(double(ret), double(0.79539883),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::ACos(val);
  EXPECT_NEAR(double(ret), double(2.6905658),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, acos_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ACos(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("1.2661037"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("1.6709638"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.79539883"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("2.6905658"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, atan)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::ATan(val);
  EXPECT_NEAR(double(ret), double(0.29145679),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::ATan(val);
  EXPECT_NEAR(double(ret), double(-0.099668652),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::ATan(val);
  EXPECT_NEAR(double(ret), double(0.61072594),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::ATan(val);
  EXPECT_NEAR(double(ret), double(-0.73281509),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, atan_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ATan(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.29145679"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("-0.099668652"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.61072594"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-0.73281509"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, sinh)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::SinH(val);
  EXPECT_NEAR(double(ret), double(0.30452031),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::SinH(val);
  EXPECT_NEAR(double(ret), double(-0.10016675),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::SinH(val);
  EXPECT_NEAR(double(ret), double(0.75858366),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::SinH(val);
  EXPECT_NEAR(double(ret), double(-1.0265167),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, sinh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::SinH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.30452031"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("-0.10016675"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.75858366"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-1.0265167"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, cosh)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::CosH(val);
  EXPECT_NEAR(double(ret), double(1.0453385),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::CosH(val);
  EXPECT_NEAR(double(ret), double(1.0050042),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::CosH(val);
  EXPECT_NEAR(double(ret), double(1.255169),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::CosH(val);
  EXPECT_NEAR(double(ret), double(1.4330864),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, cosh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::CosH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("1.0453385"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("1.0050042"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("1.255169"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("1.4330864"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, tanh)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::TanH(val);
  EXPECT_NEAR(double(ret), double(0.29131263),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::TanH(val);
  EXPECT_NEAR(double(ret), double(-0.099667996),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::TanH(val);
  EXPECT_NEAR(double(ret), double(0.60436779),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::TanH(val);
  EXPECT_NEAR(double(ret), double(-0.71629786),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, tanh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::TanH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.29131263"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("-0.099667996"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.60436779"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-0.71629786"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, asinh)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::ASinH(val);
  EXPECT_NEAR(double(ret), double(0.29567307),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::ASinH(val);
  EXPECT_NEAR(double(ret), double(-0.099834077),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::ASinH(val);
  EXPECT_NEAR(double(ret), double(0.65266657),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::ASinH(val);
  EXPECT_NEAR(double(ret), double(-0.80886692),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, asinh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ASinH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.29567307"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("-0.099834077"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.65266657"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-0.80886692"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, acosh)
{
  auto      val = fetch::math::Type<TypeParam>("1.1");
  TypeParam ret = fetch::math::ACosH(val);
  EXPECT_NEAR(double(ret), double(0.44356832),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("7.1");
  ret = fetch::math::ACosH(val);
  EXPECT_NEAR(double(ret), double(2.6482453),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("23");
  ret = fetch::math::ACosH(val);
  EXPECT_NEAR(double(ret), double(3.8281684),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("197");
  ret = fetch::math::ACosH(val);
  EXPECT_NEAR(double(ret), double(5.9763446),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, acosh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("1.1"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("7.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("23"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("197"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ACosH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.44356832"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("2.6482453"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("3.8281684"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("5.9763446"));

  ASSERT_TRUE(output.AllClose(numpy_output, function_tolerance<TypeParam>(),
                              function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, atanh)
{
  auto      val = fetch::math::Type<TypeParam>("0.3");
  TypeParam ret = fetch::math::ATanH(val);
  EXPECT_NEAR(double(ret), double(0.30951962),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.1");
  ret = fetch::math::ATanH(val);
  EXPECT_NEAR(double(ret), double(-0.10033535),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("0.7");
  ret = fetch::math::ATanH(val);
  EXPECT_NEAR(double(ret), double(0.86730051),
              4 * static_cast<double>(function_tolerance<TypeParam>()));

  val = fetch::math::Type<TypeParam>("-0.9");
  ret = fetch::math::ATanH(val);
  EXPECT_NEAR(double(ret), double(-1.4722193),
              4 * static_cast<double>(function_tolerance<TypeParam>()));
}

TYPED_TEST(TrigTest, atanh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, fetch::math::Type<TypeParam>("0.3"));
  array1.Set(zero, one, fetch::math::Type<TypeParam>("-0.1"));
  array1.Set(one, zero, fetch::math::Type<TypeParam>("0.7"));
  array1.Set(one, one, fetch::math::Type<TypeParam>("-0.9"));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ATanH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, fetch::math::Type<TypeParam>("0.30951962"));
  numpy_output.Set(zero, one, fetch::math::Type<TypeParam>("-0.10033535"));
  numpy_output.Set(one, zero, fetch::math::Type<TypeParam>("0.86730051"));
  numpy_output.Set(one, one, fetch::math::Type<TypeParam>("-1.4722193"));

  ASSERT_TRUE(output.AllClose(numpy_output,
                              fetch::math::Type<TypeParam>("4") * function_tolerance<TypeParam>(),
                              fetch::math::Type<TypeParam>("4") * function_tolerance<TypeParam>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
