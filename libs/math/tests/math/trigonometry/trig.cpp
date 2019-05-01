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

#include <iomanip>
#include <iostream>

#include "math/tensor.hpp"
#include "math/trigonometry.hpp"
#include <gtest/gtest.h>

template <typename T>
class TrigTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<float, double, fetch::fixed_point::FixedPoint<32, 32>>;
TYPED_TEST_CASE(TrigTest, MyTypes);

TYPED_TEST(TrigTest, sin)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::Sin(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.29552022));

  val = TypeParam(1.2);
  ret = fetch::math::Sin(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.93203908));

  val = TypeParam(0.7);
  ret = fetch::math::Sin(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.64421767));

  val = TypeParam(22);
  ret = fetch::math::Sin(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.0088513093));
}

TYPED_TEST(TrigTest, sin_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(1.2));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(22));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::Sin(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.29552022));
  numpy_output.Set(zero, one, TypeParam(0.93203908));
  numpy_output.Set(one, zero, TypeParam(0.64421767));
  numpy_output.Set(one, one, TypeParam(-0.0088513093));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, cos)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::Cos(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.95533651));

  val = TypeParam(1.2);
  ret = fetch::math::Cos(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.36235771));

  val = TypeParam(0.7);
  ret = fetch::math::Cos(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.76484221));

  val = TypeParam(22);
  ret = fetch::math::Cos(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.99996084));
}

TYPED_TEST(TrigTest, cos_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(1.2));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(22));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::Cos(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.95533651));
  numpy_output.Set(zero, one, TypeParam(0.36235771));
  numpy_output.Set(one, zero, TypeParam(0.76484221));
  numpy_output.Set(one, one, TypeParam(-0.99996084));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, tan)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::Tan(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.30933625));

  val = TypeParam(1.2);
  ret = fetch::math::Tan(val);
  EXPECT_FLOAT_EQ(float(ret), float(2.5721519));

  val = TypeParam(0.7);
  ret = fetch::math::Tan(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.84228837));

  val = TypeParam(22);
  ret = fetch::math::Tan(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.0088516558));
}

TYPED_TEST(TrigTest, tan_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(1.2));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(22));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::Tan(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.30933625));
  numpy_output.Set(zero, one, TypeParam(2.5721519));
  numpy_output.Set(one, zero, TypeParam(0.84228837));
  numpy_output.Set(one, one, TypeParam(0.0088516558));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, asin)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::ASin(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.30469266));

  val = TypeParam(-0.1);
  ret = fetch::math::ASin(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.10016742));

  val = TypeParam(0.7);
  ret = fetch::math::ASin(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.77539748));

  val = TypeParam(-0.9);
  ret = fetch::math::ASin(val);
  EXPECT_FLOAT_EQ(float(ret), float(-1.1197695));
}

TYPED_TEST(TrigTest, asin_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ASin(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.30469266));
  numpy_output.Set(zero, one, TypeParam(-0.10016742));
  numpy_output.Set(one, zero, TypeParam(0.77539748));
  numpy_output.Set(one, one, TypeParam(-1.1197695));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, acos)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::ACos(val);
  EXPECT_FLOAT_EQ(float(ret), float(1.2661037));

  val = TypeParam(-0.1);
  ret = fetch::math::ACos(val);
  EXPECT_FLOAT_EQ(float(ret), float(1.6709638));

  val = TypeParam(0.7);
  ret = fetch::math::ACos(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.79539883));

  val = TypeParam(-0.9);
  ret = fetch::math::ACos(val);
  EXPECT_FLOAT_EQ(float(ret), float(2.6905658));
}

TYPED_TEST(TrigTest, acos_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ACos(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(1.2661037));
  numpy_output.Set(zero, one, TypeParam(1.6709638));
  numpy_output.Set(one, zero, TypeParam(0.79539883));
  numpy_output.Set(one, one, TypeParam(2.6905658));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, atan)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::ATan(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.29145679));

  val = TypeParam(-0.1);
  ret = fetch::math::ATan(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.099668652));

  val = TypeParam(0.7);
  ret = fetch::math::ATan(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.61072594));

  val = TypeParam(-0.9);
  ret = fetch::math::ATan(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.73281509));
}

TYPED_TEST(TrigTest, atan_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ATan(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.29145679));
  numpy_output.Set(zero, one, TypeParam(-0.099668652));
  numpy_output.Set(one, zero, TypeParam(0.61072594));
  numpy_output.Set(one, one, TypeParam(-0.73281509));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, sinh)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::SinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.30452031));

  val = TypeParam(-0.1);
  ret = fetch::math::SinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.10016675));

  val = TypeParam(0.7);
  ret = fetch::math::SinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.75858366));

  val = TypeParam(-0.9);
  ret = fetch::math::SinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-1.0265167));
}

TYPED_TEST(TrigTest, sinh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::SinH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.30452031));
  numpy_output.Set(zero, one, TypeParam(-0.10016675));
  numpy_output.Set(one, zero, TypeParam(0.75858366));
  numpy_output.Set(one, one, TypeParam(-1.0265167));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, cosh)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::CosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(1.0453385));

  val = TypeParam(-0.1);
  ret = fetch::math::CosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(1.0050042));

  val = TypeParam(0.7);
  ret = fetch::math::CosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(1.255169));

  val = TypeParam(-0.9);
  ret = fetch::math::CosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(1.4330864));
}

TYPED_TEST(TrigTest, cosh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::CosH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(1.0453385));
  numpy_output.Set(zero, one, TypeParam(1.0050042));
  numpy_output.Set(one, zero, TypeParam(1.255169));
  numpy_output.Set(one, one, TypeParam(1.4330864));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, tanh)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::TanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.29131263));

  val = TypeParam(-0.1);
  ret = fetch::math::TanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.099667996));

  val = TypeParam(0.7);
  ret = fetch::math::TanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.60436779));

  val = TypeParam(-0.9);
  ret = fetch::math::TanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.71629786));
}

TYPED_TEST(TrigTest, tanh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::TanH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.29131263));
  numpy_output.Set(zero, one, TypeParam(-0.099667996));
  numpy_output.Set(one, zero, TypeParam(0.60436779));
  numpy_output.Set(one, one, TypeParam(-0.71629786));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, asinh)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::ASinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.29567307));

  val = TypeParam(-0.1);
  ret = fetch::math::ASinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.099834077));

  val = TypeParam(0.7);
  ret = fetch::math::ASinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.65266657));

  val = TypeParam(-0.9);
  ret = fetch::math::ASinH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.80886692));
}

TYPED_TEST(TrigTest, asinh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ASinH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.29567307));
  numpy_output.Set(zero, one, TypeParam(-0.099834077));
  numpy_output.Set(one, zero, TypeParam(0.65266657));
  numpy_output.Set(one, one, TypeParam(-0.80886692));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, acosh)
{
  TypeParam val = TypeParam(1.1);
  TypeParam ret = fetch::math::ACosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.44356832));

  val = TypeParam(7.1);
  ret = fetch::math::ACosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(2.6482453));

  val = TypeParam(23);
  ret = fetch::math::ACosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(3.8281684));

  val = TypeParam(197);
  ret = fetch::math::ACosH(val);
  EXPECT_FLOAT_EQ(float(ret), float(5.9763446));
}

TYPED_TEST(TrigTest, acosh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(1.1));
  array1.Set(zero, one, TypeParam(7.1));
  array1.Set(one, zero, TypeParam(23));
  array1.Set(one, one, TypeParam(197));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ACosH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.44356832));
  numpy_output.Set(zero, one, TypeParam(2.6482453));
  numpy_output.Set(one, zero, TypeParam(3.8281684));
  numpy_output.Set(one, one, TypeParam(5.9763446));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

TYPED_TEST(TrigTest, atanh)
{
  TypeParam val = TypeParam(0.3);
  TypeParam ret = fetch::math::ATanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.30951962));

  val = TypeParam(-0.1);
  ret = fetch::math::ATanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-0.10033535));

  val = TypeParam(0.7);
  ret = fetch::math::ATanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(0.86730051));

  val = TypeParam(-0.9);
  ret = fetch::math::ATanH(val);
  EXPECT_FLOAT_EQ(float(ret), float(-1.4722193));
}

TYPED_TEST(TrigTest, atanh_22)
{
  using SizeType = fetch::math::SizeType;
  SizeType zero{0};
  SizeType one{1};

  fetch::math::Tensor<TypeParam> array1{{2, 2}};

  array1.Set(zero, zero, TypeParam(0.3));
  array1.Set(zero, one, TypeParam(-0.1));
  array1.Set(one, zero, TypeParam(0.7));
  array1.Set(one, one, TypeParam(-0.9));

  fetch::math::Tensor<TypeParam> output{{2, 2}};
  fetch::math::ATanH(array1, output);

  fetch::math::Tensor<TypeParam> numpy_output{{2, 2}};

  numpy_output.Set(zero, zero, TypeParam(0.30951962));
  numpy_output.Set(zero, one, TypeParam(-0.10033535));
  numpy_output.Set(one, zero, TypeParam(0.86730051));
  numpy_output.Set(one, one, TypeParam(-1.4722193));

  ASSERT_TRUE(output.AllClose(numpy_output));
}
