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
#include "ledger/upow/work.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::byte_array::ConstByteArray;
using fetch::ledger::WorkQueue;
using fetch::ledger::Work;
using fetch::ledger::WorkScore;
using fetch::ledger::WorkPtr;

using UInt256 = Work::UInt256;
using WorkQueuePtr = std::unique_ptr<WorkQueue>;

class WorkQueueTests : public ::testing::Test
{
protected:

  void SetUp() override
  {
    work_queue_ = std::make_unique<WorkQueue>();
  }

  void TearDown() override
  {
    work_queue_.reset();
  }

  WorkPtr CreateWork(WorkScore score, uint64_t nonce = 0)
  {
    auto work = std::make_shared<Work>();
    work->UpdateScore(score);
    work->UpdateNonce(UInt256{nonce});
    return work;
  }

  WorkQueuePtr work_queue_;
};

TEST_F(WorkQueueTests, CheckBasicOrdering)
{
  // add items into the work queue
  work_queue_->push(CreateWork(500));
  work_queue_->push(CreateWork(200));
  work_queue_->push(CreateWork(100));
  work_queue_->push(CreateWork(400));
  work_queue_->push(CreateWork(300));

  // ensure items come out of queue in the correct order
  {
    auto item = work_queue_->top();
    work_queue_->pop();

    EXPECT_EQ(item->score(), 100);
  }

  {
    auto item = work_queue_->top();
    work_queue_->pop();

    EXPECT_EQ(item->score(), 200);
  }

  {
    auto item = work_queue_->top();
    work_queue_->pop();

    EXPECT_EQ(item->score(), 300);
  }

  {
    auto item = work_queue_->top();
    work_queue_->pop();

    EXPECT_EQ(item->score(), 400);
  }

  {
    auto item = work_queue_->top();
    work_queue_->pop();

    EXPECT_EQ(item->score(), 500);
  }
}

TEST_F(WorkQueueTests, CheckOrderingWhenSameScore)
{
  auto item1 = CreateWork(2000, 12);
  auto item2 = CreateWork(2000, 12321321);

  work_queue_->push(item2);
  work_queue_->push(item1);

  auto first = work_queue_->top();
  work_queue_->pop();

  auto second = work_queue_->top();
  work_queue_->pop();

  EXPECT_EQ(first.get(), item1.get());
  EXPECT_EQ(second.get(), item2.get());
}

} // namespace