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

#include "network/details/thread_pool.hpp"

#include "gmock/gmock.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

namespace {

using fetch::network::MakeThreadPool;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;
using ::testing::AtLeast;

class Mock
{
public:
  Mock()
  {
    ON_CALL(*this, Run()).WillByDefault([this]() { ++counter; });
  }

  MOCK_METHOD0(Run, void());

  std::atomic<std::size_t> counter{0};
};

class ThreadPoolTests : public ::testing::TestWithParam<std::size_t>
{
protected:
  using ThreadPool = fetch::network::ThreadPool;
  using MockPtr    = std::unique_ptr<Mock>;

  void SetUp() override
  {
    mock_ = std::make_unique<Mock>();
    pool_ = MakeThreadPool(GetParam());
    pool_->Start();
  }

  bool WaitForCompletion(std::size_t min_count)
  {
    using Clock     = std::chrono::high_resolution_clock;
    using Timestamp = Clock::time_point;

    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    bool            success  = false;
    Timestamp const deadline = Clock::now() + milliseconds{8000};

    while (Clock::now() < deadline)
    {
      sleep_for(milliseconds{250});

      // exit on completion
      bool const execute_complete = (pool_->execute_count() >= min_count);
      bool const mock_complete    = (mock_->counter >= min_count);

      if (execute_complete && mock_complete)
      {
        success = true;
        break;
      }
    }

    return success;
  }

  MockPtr    mock_;
  ThreadPool pool_;
};

TEST_P(ThreadPoolTests, DISABLED_CheckBasicOperation)
{
  std::size_t const work_count = 500;

  EXPECT_CALL(*mock_, Run()).Times(work_count);

  // post some work to the thread pool
  for (std::size_t i = 0; i < work_count; ++i)
  {
    pool_->Post([this]() { mock_->Run(); });
  }

  ASSERT_TRUE(WaitForCompletion(work_count));
}

TEST_P(ThreadPoolTests, DISABLED_CheckFutureOperation)
{
  std::size_t const work_count = 500;

  EXPECT_CALL(*mock_, Run()).Times(work_count);

  // post some work to the thread pool
  for (std::size_t i = 0; i < work_count; ++i)
  {
    pool_->Post([this]() { mock_->Run(); }, 100);
  }

  ASSERT_TRUE(WaitForCompletion(work_count));
}

TEST_P(ThreadPoolTests, DISABLED_CheckIdleWorkers)
{
  static constexpr std::size_t INTERVAL_MS         = 100;
  static constexpr std::size_t EXPECTED_ITERATIONS = 20;
  static constexpr std::size_t TEST_TIME_MS        = (INTERVAL_MS * EXPECTED_ITERATIONS * 5) / 3;

  using Clock        = std::chrono::high_resolution_clock;
  using Timepoint    = Clock::time_point;
  using ExecutionLog = std::vector<Timepoint>;

  std::mutex   log_mutex;
  ExecutionLog log;

  EXPECT_CALL(*mock_, Run()).Times(AtLeast(EXPECTED_ITERATIONS));

  pool_->SetIdleInterval(INTERVAL_MS);
  pool_->PostIdle([this, &log_mutex, &log]() {
    // update the log
    {
      std::lock_guard<std::mutex> lock(log_mutex);
      Timepoint const             now = Clock::now();
      log.push_back(now);
    }

    // update the mock
    mock_->Run();
  });

  // should be more than enough time to perform 20 periodic cycles with at 10ms intervals
  sleep_for(milliseconds{TEST_TIME_MS});

  // clear all the queues in this case, remove the periodic added before hand
  pool_->Clear();

  EXPECT_TRUE(WaitForCompletion(EXPECTED_ITERATIONS));

  // for interest print out the times between runs
  for (std::size_t idx = 1; idx < log.size(); ++idx)
  {
    // determine the delta
    auto const delta = duration_cast<milliseconds>(log[idx] - log[idx - 1]).count();

    // for very small intervals due to thread scheduling this might not actually be the case,
    // however, we always expect in general for this to be true in normal operation
    EXPECT_GE(delta, INTERVAL_MS);
  }
}

TEST_P(ThreadPoolTests, DISABLED_SaturationCheck)
{
  std::size_t const num_threads = GetParam();

  std::atomic<bool>        running{true};
  std::atomic<std::size_t> active{0};

  // spin up enough spin loops to saturate the thread pool
  for (std::size_t i = 0; i < num_threads; ++i)
  {
    pool_->Post([&running, &active]() {
      ++active;

      while (running)
      {
        // intentional spin loop
      }

      --active;
    });
  }

  // wait for the threads to all become active
  bool reached_saturation = false;
  for (std::size_t i = 0; i < 40; ++i)
  {
    // check if we have reached saturation
    if (active >= num_threads)
    {
      reached_saturation = true;
      break;
    }

    sleep_for(milliseconds{100});
  }

  ASSERT_TRUE(reached_saturation);

  // signal the threads to stop
  running = false;

  bool workers_stopped = false;
  for (std::size_t i = 0; i < 40; ++i)
  {
    // check if we have reached saturation
    if (active == 0)
    {
      workers_stopped = true;
      break;
    }

    sleep_for(milliseconds{100});
  }

  ASSERT_TRUE(workers_stopped);
}

INSTANTIATE_TEST_CASE_P(ParamBased, ThreadPoolTests, ::testing::Values(1, 10), );

}  // namespace
