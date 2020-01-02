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

#include "gtest/gtest.h"
#include "semanticsearch/index/in_memory_db_index.hpp"
using namespace fetch::semanticsearch;

TEST(SemanticSearchIndex, BasicOperations1D)
{
  InMemoryDBIndex        database_index{1};
  SemanticCoordinateType width = static_cast<SemanticCoordinateType>(-1) / 16;

  for (SemanticCoordinateType i = 0; i < 16; ++i)
  {
    SemanticSubscription rel;
    rel.position.push_back(width * i + (width >> 1));
    rel.index = i;
    database_index.AddRelation(rel);
  }

  auto group0 = database_index.Find(0, {width * 8});
  EXPECT_NE(group0, nullptr);
  EXPECT_EQ(group0->size(), 16);

  auto group1 = database_index.Find(1, {width * 4});
  EXPECT_NE(group1, nullptr);
  EXPECT_EQ(group1->size(), 8);
  EXPECT_EQ(*group1, std::set<DBIndexType>({0, 1, 2, 3, 4, 5, 6, 7}));

  auto group2 = database_index.Find(1, {width * 12});
  EXPECT_NE(group2, nullptr);
  EXPECT_EQ(group2->size(), 8);
  EXPECT_EQ(*group2, std::set<DBIndexType>({8, 9, 10, 11, 12, 13, 14, 15}));

  auto group11 = database_index.Find(2, {width * 2});
  EXPECT_NE(group11, nullptr);
  EXPECT_EQ(group11->size(), 4);
  EXPECT_EQ(*group11, std::set<DBIndexType>({0, 1, 2, 3}));

  auto group12 = database_index.Find(2, {width * 6});
  EXPECT_NE(group12, nullptr);
  EXPECT_EQ(group12->size(), 4);
  EXPECT_EQ(*group12, std::set<DBIndexType>({4, 5, 6, 7}));

  auto group21 = database_index.Find(2, {width * 10});
  EXPECT_NE(group21, nullptr);
  EXPECT_EQ(group21->size(), 4);
  EXPECT_EQ(*group21, std::set<DBIndexType>({8, 9, 10, 11}));

  auto group22 = database_index.Find(2, {width * 14});
  EXPECT_NE(group22, nullptr);
  EXPECT_EQ(group22->size(), 4);
  EXPECT_EQ(*group22, std::set<DBIndexType>({12, 13, 14, 15}));
}

TEST(SemanticSearchIndex, BasicOperations2D)
{
  InMemoryDBIndex        database_index{2};
  SemanticCoordinateType width = static_cast<SemanticCoordinateType>(-1) / 4;

  // Adding points in a grid
  for (SemanticCoordinateType i = 0; i < 4; ++i)
  {
    for (SemanticCoordinateType j = 0; j < 4; ++j)
    {
      SemanticSubscription rel;
      rel.position.push_back(width * i + (width >> 1));
      rel.position.push_back(width * j + (width >> 1));
      rel.index = i * 4 + j;
      database_index.AddRelation(rel);
    }
  }

  // Testing exception
  try
  {
    auto fail_group0 = database_index.Find(0, {width * 2});
    FAIL() << "Was expecting exception" << std::endl;
  }
  catch (std::exception const &e)
  {
  }

  // Testing search
  auto group0 = database_index.Find(0, {width * 2, width * 2});
  EXPECT_NE(group0, nullptr);
  EXPECT_EQ(group0->size(), 16);
  EXPECT_EQ(*group0, std::set<DBIndexType>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));

  auto group1 = database_index.Find(1, {width, width});
  EXPECT_NE(group1, nullptr);
  EXPECT_EQ(group1->size(), 4);
  EXPECT_EQ(*group1, std::set<DBIndexType>({0, 1, 4, 5}));

  auto group2 = database_index.Find(1, {width, 3 * width});
  EXPECT_NE(group2, nullptr);
  EXPECT_EQ(group2->size(), 4);
  EXPECT_EQ(*group2, std::set<DBIndexType>({2, 3, 6, 7}));

  auto group3 = database_index.Find(1, {3 * width, width});
  EXPECT_NE(group3, nullptr);
  EXPECT_EQ(group3->size(), 4);
  EXPECT_EQ(*group3, std::set<DBIndexType>({8, 9, 12, 13}));

  auto group4 = database_index.Find(1, {3 * width, 3 * width});
  EXPECT_NE(group4, nullptr);
  EXPECT_EQ(group4->size(), 4);
  EXPECT_EQ(*group4, std::set<DBIndexType>({10, 11, 14, 15}));
}
