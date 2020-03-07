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

#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::telemetry::Registry;

TEST(RegistryTests, Uniqueness)
{
  {
    auto ctr = Registry::Instance().CreateCounter("test_total", "This counter should be unique");
    ASSERT_EQ(ctr->count(), 0);
    ctr->increment();
    ASSERT_EQ(ctr->count(), 1);
  }
  {
    auto ctr1 = Registry::Instance().CreateCounter("test_total", "This counter should be unique");
    ASSERT_EQ(ctr1->count(), 1);
    ctr1->increment();
    ASSERT_EQ(ctr1->count(), 2);
  }
}

using fetch::telemetry::Counter;

TEST(RegistryTests, MultipleLabels)
{
  auto ctr1 = Registry::Instance().CreateCounter("with_labels_total", "This should not",
                                                 Registry::Labels{{"Hello", "world"}});
  auto ctr2 = Registry::Instance().CreateCounter("with_labels_total", "This should not",
                                                 Registry::Labels{{"Answer", "42"}});
  ASSERT_NE(ctr1, ctr2);
  auto ctr = Registry::Instance().LookupMeasurement<Counter>("with_labels_total");
  ASSERT_TRUE(ctr == ctr1 || ctr == ctr2);
}

}  // namespace
