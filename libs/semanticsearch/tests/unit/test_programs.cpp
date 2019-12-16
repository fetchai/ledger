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
#include "semanticsearch/index/in_memory_db_index.hpp"
#include "toolkit.hpp"
using namespace fetch::semanticsearch;

TEST(SemanticSearchIndex, TestPrograms)
{
  SemanticSearchToolkit toolkit;

  toolkit.RegisterAgent("agent_pk");

  auto query = toolkit.Compile(R"(model IntPair {
  key1: Integer,
  key2: Integer
};

let y : IntPair = {
    key1: 9,
    key2: 20
};

advertise y;)",
                               "test.search");
  EXPECT_FALSE(toolkit.HasErrors());
  toolkit.Execute(query, "agent_pk");
  EXPECT_FALSE(toolkit.HasErrors());
  if (toolkit.HasErrors())
  {
    toolkit.PrintErrors();
  }

  query = toolkit.Compile(R"(model IntPair {
  key1: BoundedInteger(8, 20),
  key2: BoundedInteger(20, 40)  
};

let y : IntPair = {
    key1: 9,
    key2: 20
};

advertise y;)",
                          "test.search");
  EXPECT_FALSE(toolkit.HasErrors());
  toolkit.Execute(query, "agent_pk");
  EXPECT_FALSE(toolkit.HasErrors());
  if (toolkit.HasErrors())
  {
    toolkit.PrintErrors();
  }

  query = toolkit.Compile(R"(model IntPair {
  key1: BoundedInteger(8, 20),
  key2: BoundedInteger(20, 40)  
};

let y : IntPair = {
    key1: 7,
    key2: 20
};

advertise y;)",
                          "test.search");
  EXPECT_FALSE(toolkit.HasErrors());
  toolkit.Execute(query, "agent_pk");
  EXPECT_TRUE(toolkit.HasErrors());
  if (toolkit.HasErrors())
  {
    toolkit.PrintErrors();
  }
}
