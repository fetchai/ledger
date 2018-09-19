#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "network/generics/threadsafe_set.hpp"
#include "network/service/promise.hpp"
#include <functional>

namespace fetch {

namespace network {

/* A collection of promises with callbacks for completed ones. */

class Promises
{
public:
  enum Conclusion
  {
    NONE,
    DONE
  };

  using individual_cb_type = std::function<void(fetch::service::Promise *)>;
  using final_cb_type = std::function<void(const std::set<fetch::service::Promise *> promises)>;

  Promises()
  {}

  Promises(Promises &&other)
    : all_promises_(std::move(other.all_promises_))
    , finished_promises_(std::move(other.finished_promises_))
    , conclusion_(other.conclusion_.load())
    , on_each_(other.on_each_)
    , on_complete_(other.on_complete_)
  {}

  Promises &Every(individual_cb_type cb)
  {
    on_each_ = cb;
    return *this;
  }

  Promises &Then(final_cb_type cb)
  {
    on_complete_ = cb;
    return *this;
  }

  Promises &add(fetch::service::Promise *p)
  {
    if (all_promises_.Add(p))
    {
      p->Then([this, p]() { this->SignalDone(p); }).Else([this, p]() { this->SignalDone(p); });
    }
    return *this;
  }

  virtual ~Promises()
  {
    all_promises_.VisitRemove([](fetch::service::Promise *p) { delete p; });
  }

private:
  void SignalDone(fetch::service::Promise *p)
  {
    if (finished_promises_.Add(p))
    {
      if (on_each_)
      {
        on_each_(p);
      }
      TryConclude();
    }
  }

  void TryConclude()
  {
    if (all_promises_.size() == finished_promises_.size())
    {
      Conclusion expected = NONE;
      if (conclusion_.compare_exchange_strong(expected, DONE))
      {
        if (on_complete_)
        {
          on_complete_(finished_promises_.GetLocked());
        }
      }
    }
  }

  generics::ThreadsafeSet<fetch::service::Promise *> all_promises_;
  generics::ThreadsafeSet<fetch::service::Promise *> finished_promises_;
  std::atomic<Conclusion>                            conclusion_{NONE};
  individual_cb_type                                 on_each_;
  final_cb_type                                      on_complete_;
};

}  // namespace network
}  // namespace fetch
