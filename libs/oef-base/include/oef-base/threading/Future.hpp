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

#include "core/mutex.hpp"
#include "oef-base/threading/Waitable.hpp"

#include <atomic>

namespace fetch {
namespace oef {
namespace base {

template <class T>
class Future : public Waitable
{
public:
  Future()
    : Waitable()
  {}

  ~Future() override = default;

  void set(const T &value)
  {
    value_.store(value);
    this->wake();
  }

  T get()
  {
    return value_.load();
  }

protected:
  std::atomic<T> value_;

private:
  Future(const Future &other) = delete;              // { copy(other); }
  Future &operator=(const Future &other)  = delete;  // { copy(other); return *this; }
  bool    operator==(const Future &other) = delete;  // const { return compare(other)==0; }
  bool    operator<(const Future &other)  = delete;  // const { return compare(other)==-1; }
};

template <class T>
class FutureComplexType : public Waitable
{
public:
  FutureComplexType()
    : Waitable()
  {}

  ~FutureComplexType() override = default;

  void set(const T &value)
  {
    {
      FETCH_LOCK(value_mutex_);
      value_ = value;
    }
    this->wake();
  }

  T get()
  {
    FETCH_LOCK(value_mutex_);
    return value_;
  }

protected:
  T          value_;
  std::mutex value_mutex_;

private:
  FutureComplexType(const FutureComplexType &other) = delete;  // { copy(other); }
  FutureComplexType &operator                       =(const FutureComplexType &other) =
      delete;                                                // { copy(other); return *this; }
  bool operator==(const FutureComplexType &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const FutureComplexType &other)  = delete;  // const { return compare(other)==-1; }
};

}  // namespace base
}  // namespace oef
}  // namespace fetch
