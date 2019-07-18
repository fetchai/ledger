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

#include "core/feature_flags.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::core::FeatureFlags;
using fetch::byte_array::ConstByteArray;

using FeatureFlagsPtr = std::unique_ptr<FeatureFlags>;

class FeatureFlagTests : public ::testing::Test
{
protected:
  virtual void SetUp()
  {
    flags_ = std::make_unique<FeatureFlags>();
  }

  virtual void TearDown()
  {
    flags_.reset();
  }

  ::testing::AssertionResult CheckContents(std::unordered_set<ConstByteArray> const &items) const
  {
    if (items.size() != flags_->size())
    {
      return ::testing::AssertionFailure()
             << "Unexpected size " << items.size() << " vs. " << flags_->size();
    }

    for (auto const &item : *flags_)
    {
      if (items.find(item) == items.end())
      {
        return ::testing::AssertionFailure() << "Missing feature: " << item;
      }
    }

    return ::testing::AssertionSuccess();
  }

  FeatureFlagsPtr flags_;
};

TEST_F(FeatureFlagTests, Simple)
{
  flags_->Parse("foo,bar,baz");
  EXPECT_TRUE(CheckContents({"foo", "bar", "baz"}));
}

}  // namespace
