#pragma once
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
#include "math/base_types.hpp"
#include <mutex>

enum class CoordinatorMode
{
  SYNCHRONOUS,
  SEMI_SYNCHRONOUS,
  ASYNCHRONOUS
};

enum class CoordinatorState
{
  RUN,
  STOP,
};

class Coordinator
{
public:
  using SizeType = fetch::math::SizeType;

  Coordinator(CoordinatorMode mode, SizeType iterations_count = 0)
    : mode_(mode)
    , iterations_count_(iterations_count)
  {}

  void IncrementIterationsCounter()
  {
    std::lock_guard<std::mutex> l(iterations_mutex_);
    iterations_done_++;

    if (iterations_done_ >= iterations_count_)
    {
      state_ = CoordinatorState::STOP;
    }
  }

  void Reset()
  {
    iterations_done_ = 0;
    state_           = CoordinatorState::RUN;
  }

  CoordinatorMode GetMode() const
  {
    return mode_;
  }

  CoordinatorState GetState() const
  {
    return state_;
  }

private:
  CoordinatorMode  mode_;
  CoordinatorState state_           = CoordinatorState::RUN;
  SizeType         iterations_done_ = 0;
  SizeType         iterations_count_;
  std::mutex       iterations_mutex_;
};
