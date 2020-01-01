#pragma once
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

#include "core/runnable.hpp"
#include "network/service/promise.hpp"

namespace fetch {
namespace muddle {

class PromiseTask : public core::Runnable
{
public:
  using Callback  = std::function<void(service::Promise const &)>;
  using Clock     = service::details::PromiseImplementation::Clock;
  using Duration  = Clock::duration;
  using Timepoint = Clock::time_point;

  // Construction / Destruction
  template <typename Class>
  PromiseTask(service::Promise const &promise, Class *instance,
              void (Class::*member_function)(service::Promise const &));
  template <typename Class>
  PromiseTask(service::Promise const &promise, Duration const &timeout, Class *instance,
              void (Class::*member_function)(service::Promise const &));
  PromiseTask(service::Promise const &promise, Callback callback);
  PromiseTask(service::Promise const &promise, Duration const &timeout, Callback callback);
  PromiseTask(PromiseTask const &) = delete;
  PromiseTask(PromiseTask &&)      = delete;
  ~PromiseTask() override          = default;

  /// @name Runnable
  /// @{
  bool        IsReadyToExecute() const final;
  void        Execute() final;
  char const *GetId() const final;
  /// @}

  bool IsComplete() const;

  // Operators
  PromiseTask &operator=(PromiseTask const &) = delete;
  PromiseTask &operator=(PromiseTask &&) = delete;

private:
  PromiseTask(service::Promise promise, Timepoint const &deadline, Callback callback);

  service::Promise promise_;
  Timepoint        deadline_;
  Callback         callback_;
  bool             complete_{false};
};

template <typename Class>
PromiseTask::PromiseTask(service::Promise const &promise, Class *instance,
                         void (Class::*member_function)(service::Promise const &))
  : PromiseTask(promise, [instance, member_function](service::Promise const &p) {
    (instance->*member_function)(p);
  })
{}

template <typename Class>
PromiseTask::PromiseTask(service::Promise const &promise, Duration const &timeout, Class *instance,
                         void (Class::*member_function)(service::Promise const &))
  : PromiseTask(promise, timeout, [instance, member_function](service::Promise const &p) {
    (instance->*member_function)(p);
  })
{}

}  // namespace muddle
}  // namespace fetch
