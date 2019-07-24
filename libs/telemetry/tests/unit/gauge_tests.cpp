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

#include "telemetry/gauge.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <memory>

namespace {

using fetch::telemetry::Measurement;
using fetch::telemetry::OutputStream;

using IntegerTypes = ::testing::Types<uint32_t, uint64_t>;
using FloatTypes   = ::testing::Types<float, double>;
using AllTypes     = ::testing::Types<uint32_t, uint64_t, float, double>;

template <typename T>
class GeneralGaugeTests : public ::testing::Test
{
protected:
  using GaugeType = fetch::telemetry::Gauge<T>;
  using GaugePtr  = std::unique_ptr<GaugeType>;

  void SetUp() override
  {
    gauge_ = std::make_unique<GaugeType>("sample_gauge", "Description of gauge");
  }

  void TearDown() override
  {
    gauge_.reset();
  }

  GaugePtr gauge_;
};

template <typename T>
class IntegerGaugeTests : public GeneralGaugeTests<T>
{
};

template <typename T>
class FloatGaugeTests : public GeneralGaugeTests<T>
{
};

TYPED_TEST_CASE(IntegerGaugeTests, IntegerTypes);
TYPED_TEST_CASE(FloatGaugeTests, FloatTypes);
TYPED_TEST_CASE(GeneralGaugeTests, AllTypes);

TYPED_TEST(IntegerGaugeTests, CheckIncrement)
{
  EXPECT_EQ(this->gauge_->get(), 0);
  this->gauge_->increment();
  EXPECT_EQ(this->gauge_->get(), 1);
}

TYPED_TEST(IntegerGaugeTests, CheckDecrement)
{
  this->gauge_->set(100);
  this->gauge_->decrement();
  EXPECT_EQ(this->gauge_->get(), 99);
}

TYPED_TEST(IntegerGaugeTests, CheckAdd)
{
  this->gauge_->set(2);
  this->gauge_->increment(2);
  EXPECT_EQ(this->gauge_->get(), 4);
}

TYPED_TEST(IntegerGaugeTests, CheckRemove)
{
  this->gauge_->set(4);
  this->gauge_->decrement(2);
  EXPECT_EQ(this->gauge_->get(), 2);
}

TYPED_TEST(IntegerGaugeTests, CheckSerialisation)
{
  this->gauge_->set(200);
  EXPECT_EQ(this->gauge_->get(), 200);

  static char const *EXPECTED_TEXT = R"(# HELP sample_gauge Description of gauge
# TYPE sample_gauge gauge
sample_gauge 200
)";

  std::ostringstream oss;
  OutputStream stream{oss};
  this->gauge_->ToStream(stream);

  EXPECT_EQ(oss.str(), std::string{EXPECTED_TEXT});
}

TYPED_TEST(GeneralGaugeTests, SetValue)
{
  this->gauge_->set(2);
  EXPECT_EQ(this->gauge_->get(), 2);
}

TYPED_TEST(FloatGaugeTests, CheckSerialisation)
{
  this->gauge_->set(3.1456f);
  EXPECT_FLOAT_EQ(static_cast<float>(this->gauge_->get()), 3.1456f);

  static char const *EXPECTED_TEXT = R"(# HELP sample_gauge Description of gauge
# TYPE sample_gauge gauge
sample_gauge 3.145600e+00
)";

  std::ostringstream oss;
  OutputStream stream{oss};
  this->gauge_->ToStream(stream);

  EXPECT_EQ(oss.str(), std::string{EXPECTED_TEXT});
}

}  // namespace
