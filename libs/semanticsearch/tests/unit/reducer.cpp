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

    ret.PushBack(static_cast<SemanticCoordinateType>(SemanticCoordinateType::FP_MAX * val));
    return ret;
  });

  reducer.SetValidator<double>([from, to](double const &val, std::string & /*error*/) {
    return (from <= val) && (val <= to);
  });

  std::string error;
  EXPECT_FALSE(reducer.Validate<double>(92., error));
  EXPECT_FALSE(reducer.Validate<double>(-192., error));
  EXPECT_TRUE(reducer.Validate<double>(3., error));
  EXPECT_TRUE(reducer.Validate<double>(90., error));
  EXPECT_TRUE(reducer.Validate<double>(-90., error));

  // TODO:  EXPECT_EQ(reducer.Reduce<double>(-90.), SemanticPosition({0}));
  // Note that due to rounding errors, we don't hit the exact limit
  // uint64(-1)
  EXPECT_LE(SemanticCoordinateType::FP_MAX - reducer.Reduce<double>(90.)[0],
            SemanticCoordinateType::FP_MAX / 180);
}
