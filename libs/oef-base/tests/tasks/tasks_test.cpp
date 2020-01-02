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

#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/threading/Threadpool.hpp"

class TasksTests : public testing::Test
{
public:
  using Taskpool    = fetch::oef::base::Taskpool;
  using Threadpool  = fetch::oef::base::Threadpool;
  using TaskpoolP   = std::shared_ptr<Taskpool>;
  using ThreadpoolP = std::shared_ptr<Threadpool>;
  using Task        = fetch::oef::base::Task;
  using TaskP       = Taskpool::TaskP;

  TaskpoolP   taskpool_;
  ThreadpoolP tasks_runners_;

  TasksTests()
  {
    taskpool_      = std::make_shared<Taskpool>();
    tasks_runners_ = std::make_shared<Threadpool>();
    std::function<void(std::size_t thread_number)> run_tasks =
        std::bind(&Taskpool::run, taskpool_.get(), std::placeholders::_1);
    tasks_runners_->start(5, run_tasks);
  }
  virtual ~TasksTests()
  {
    taskpool_->stop();
    ::usleep(10000);
    tasks_runners_->stop();
  }
};

class LambdaTask : public fetch::oef::base::Task
{
public:
  using ExitState = fetch::oef::base::ExitState;
  using Function  = std::function<ExitState()>;

  Function f_;

  LambdaTask(Function f)
    : f_(f)
  {}
  virtual ~LambdaTask()
  {}

  bool IsRunnable() const override
  {
    return true;
  }

  ExitState run() override
  {
    return f_();
  }

  LambdaTask(LambdaTask const &other) = delete;
  LambdaTask &operator=(LambdaTask const &other)  = delete;
  bool        operator==(LambdaTask const &other) = delete;
  bool        operator<(LambdaTask const &other)  = delete;
};

TEST_F(TasksTests, task_execution)
{
  unsigned int counter = 0;

  LambdaTask::Function f = [&counter]() {
    counter += 1;
    switch (counter)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      return LambdaTask::ExitState::RERUN;
    default:
      return LambdaTask::ExitState::COMPLETE;
    }
  };

  auto task = std::make_shared<LambdaTask>(f);

  taskpool_->submit(task);

  ::usleep(10000);

  EXPECT_EQ(counter, 5);
}
