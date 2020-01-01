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

#include "oef-base/threading/Waitable.hpp"
#include <atomic>

namespace fetch {
namespace oef {
namespace base {

template <class T, typename RESULT_TYPE>
class FutureCombiner : public Waitable,
                       public std::enable_shared_from_this<FutureCombiner<T, RESULT_TYPE>>
{
public:
  static constexpr char const *LOGGING_NAME = "FutureCombiner";

  using ResultMerger =
      std::function<void(std::shared_ptr<RESULT_TYPE> &, const std::shared_ptr<RESULT_TYPE> &)>;

  FutureCombiner()
    : Waitable()
    , result_{std::make_shared<RESULT_TYPE>()}
    , completed_{0}
  {}

  ~FutureCombiner() override = default;

  void AddFuture(std::shared_ptr<T> future)
  {
    auto                                          this_sp = this->shared_from_this();
    std::weak_ptr<FutureCombiner<T, RESULT_TYPE>> this_wp = this_sp;

    std::size_t future_idx = 0;
    {
      FETCH_LOCK(mutex_);

      future_idx = futures_.size();
      futures_.push_back(future);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Added future: idx=", future_idx);

    future->MakeNotification().Then([this_wp, future_idx]() {
      FETCH_LOG_INFO(LOGGING_NAME, "Got future! idx = ", future_idx);
      auto sp = this_wp.lock();
      if (sp)
      {
        try
        {
          std::lock_guard<std::mutex> lock(sp->mutex_);
          auto                        res_future = sp->futures_.at(future_idx)->get();
          if (res_future)
          {
            sp->future_combiner_(sp->result_, res_future);
          }
          else
          {
            FETCH_LOG_WARN(LOGGING_NAME, "Got nullptr from future: ", future_idx);
          }
          ++(sp->completed_);
          if (sp->futures_.size() == sp->completed_)
          {
            sp->wake();
            sp->futures_.clear();
          }
        }
        catch (std::exception const &e)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Exception while processing new future result: ", e.what());
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to lock weak ptr! Future ignored!");
      }
    });
  }

  void SetResultMerger(ResultMerger futureCombiner)
  {
    FETCH_LOCK(mutex_);
    future_combiner_ = std::move(futureCombiner);
  }

  std::shared_ptr<RESULT_TYPE> &Get()
  {
    return result_;
  }

protected:
  std::mutex                      mutex_{};
  std::vector<std::shared_ptr<T>> futures_{};
  std::shared_ptr<RESULT_TYPE>    result_;
  ResultMerger                    future_combiner_;
  uint32_t                        completed_;

private:
  FutureCombiner(const FutureCombiner &other) = delete;  // { copy(other); }
  FutureCombiner &operator                    =(const FutureCombiner &other) =
      delete;                                             // { copy(other); return *this; }
  bool operator==(const FutureCombiner &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const FutureCombiner &other)  = delete;  // const { return compare(other)==-1; }
};

}  // namespace base
}  // namespace oef
}  // namespace fetch
