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

#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/unigram_table.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

TEST(UnigramTableTest, simple_test)
{
  using SizeType = fetch::math::SizeType;

  std::vector<SizeType> counts   = {2, 1, 5};
  std::vector<SizeType> table_gt = {0, 0, 0, 1, 1, 2, 2, 2, 2, 2};

  SizeType table_size{table_gt.size()};

  UnigramTable ut(counts, table_size);

  std::vector<SizeType> table = ut.GetTable();

  EXPECT_EQ(table, table_gt);
}

TEST(UnigramTableTest, test_with_zero_counts)
{
  using SizeType = fetch::math::SizeType;

  std::vector<SizeType> counts   = {2, 1, 0, 5, 0};
  std::vector<SizeType> table_gt = {0, 0, 0, 1, 1, 3, 3, 3, 3};

  SizeType table_size{table_gt.size()};

  UnigramTable ut(counts, table_size);

  std::vector<SizeType> table = ut.GetTable();

  EXPECT_EQ(table, table_gt);
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
