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

#include "gtest/gtest.h"
#include "semanticsearch/schema/semantic_reducer.hpp"
using namespace fetch::semanticsearch;

TEST(SemanticSearchIndex, SemanticReducer)
{
  SemanticReducer reducer{"testreducer"};

  // Creating validator
  double from = -90., to = 90.;
  reducer.SetReducer<double>(1, [from, to](double val) {
    SemanticPosition ret;

    val -= from;
    val /= (to - from + 1);  // Plus one is needed for equality in validator

    ret.push_back(static_cast<uint64_t>(val * uint64_t(-1)));
    return ret;
  });

  reducer.SetValidator<double>(
      [from, to](double const &val) { return (from <= val) && (val <= to); });

  EXPECT_FALSE(reducer.Validate<double>(92.));
  EXPECT_FALSE(reducer.Validate<double>(-192.));
  EXPECT_TRUE(reducer.Validate<double>(3.));
  EXPECT_TRUE(reducer.Validate<double>(90.));
  EXPECT_TRUE(reducer.Validate<double>(-90.));

  EXPECT_EQ(reducer.Reduce<double>(-90.), std::vector<uint64_t>({0}));
  // Note that due to rounding errors, we don't hit the exact limit
  // uint64(-1)
  EXPECT_LE(static_cast<uint64_t>(-1) - reducer.Reduce<double>(90.)[0],
            static_cast<uint64_t>(-1) / 180);
}
