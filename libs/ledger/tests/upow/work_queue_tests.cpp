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

#include "ledger/upow/work_queue.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using namespace fetch::ledger;

using WorkArray    = std::vector<WorkPtr>;
using WorkArrayPtr = std::unique_ptr<WorkArray>;

class WorkQueueTests : public ::testing::Test
{
protected:

  void SetUp() override
  {
    work_queue_ = std::make_shared<WorkQueue>();
  }

  void IntoArray()
  {
    work_array_ = std::make_unique<WorkArray>();
    work_array_->reserve(work_queue_->size());

    while (!work_queue_->empty())
    {
      work_array_->emplace_back(work_queue_->top());
      work_queue_->pop();
    }
  }

  WorkPtr CreateWork(int64_t score)
  {
    // create empty work with a score set
    auto work = std::make_shared<Work>();
    work->UpdateScore(score);

    return work;
  }

  WorkQueuePtr work_queue_;
  WorkArrayPtr work_array_;
};

TEST_F(WorkQueueTests, SimpleTest)
{
  work_queue_->push(CreateWork(100));
  work_queue_->push(CreateWork(50));
  work_queue_->push(CreateWork(-1));

  IntoArray();

  ASSERT_EQ(work_array_->size(), 3);
  EXPECT_EQ(work_array_->at(0)->score(), -1);
  EXPECT_EQ(work_array_->at(1)->score(), 50);
  EXPECT_EQ(work_array_->at(2)->score(), 100);
}

} // namespace