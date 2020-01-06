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

#include "yaml/document.hpp"
#include "yaml/exceptions.hpp"
#include "yaml_test_cases.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <ostream>
#include <sstream>

using namespace fetch::yaml;
using namespace fetch::byte_array;

class YamlTests : public ::testing::TestWithParam<TestCase>
{
};

TEST_P(YamlTests, CheckParsing)
{
  TestCase const &config = GetParam();

  YamlDocument       doc;
  std::ostringstream ss;

  bool did_throw = false;

  try
  {
    doc.Parse(config.input_text);
  }
  catch (fetch::yaml::YamlParseException const &)
  {
    did_throw = true;
  }

  EXPECT_EQ(config.expect_throw, did_throw);
  if (config.expect_output)
  {
    ss << doc.root();
    EXPECT_EQ(config.output_text, ss.str());
  }
}

INSTANTIATE_TEST_CASE_P(ParamBased, YamlTests, testing::ValuesIn(TEST_CASES), );
