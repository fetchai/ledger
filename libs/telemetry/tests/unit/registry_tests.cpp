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

#include "telemetry/registry.hpp"
#include "telemetry/counter.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::telemetry::Registry;

using RegistryPtr = std::unique_ptr<Registry>;

class RegistryTests : public ::testing::Test
{
protected:

  void SetUp() override
  {
    registry_ = std::make_unique<Registry>();
  }

  void TearDown() override
  {
    registry_.reset();
  }

  RegistryPtr registry_;
};


TEST_F(RegistryTests, SimpleCheck)
{
  auto counter = registry_->CreateCounter("foo_bar_baz", "Description");
  counter->add(200);
}

} // namespace