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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "math/base_types.hpp"

#include "gtest/gtest.h"

#include <random>

namespace fetch {
namespace serializers {

TEST(FixedPointSerialisationTest, IntegerSerialisation)
{
  for (int i(-100); i < 100; ++i)
  {
    fetch::fixed_point::FixedPoint<32, 32> a(i);
    fetch::serializers::MsgPackSerializer  b;
    b << a;
    b.seek(0);
    fetch::fixed_point::FixedPoint<32, 32> c;
    b >> c;
    EXPECT_EQ(a, c);
    EXPECT_EQ(int(a), i);
  }
}

TEST(FixedPointSerialisationTest, DecimalSerialisation)
{
  std::random_device               random_device;
  std::mt19937                     generator(random_device());
  std::uniform_real_distribution<> distribution(-100.0, 100.0);

  for (int i(0); i < 100; ++i)
  {
    double value = distribution(generator);
    auto   a     = fetch::math::Type<fetch::fixed_point::FixedPoint<32, 32>>(std::to_string(value));
    fetch::serializers::MsgPackSerializer b;
    b << a;
    b.seek(0);
    fetch::fixed_point::FixedPoint<32, 32> c;
    b >> c;
    EXPECT_EQ(a, c);
    EXPECT_NEAR(double(a), value, 1e-6);
  }
}

}  // namespace serializers
}  // namespace fetch
