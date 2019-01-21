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

#include <iostream>
#include <string>

namespace fetch {
namespace generics {

template <typename FUNC>
class Callbacks
{
public:
  Callbacks(const Callbacks &rhs) = delete;
  Callbacks(Callbacks &&rhs)      = delete;
  Callbacks &operator=(const Callbacks &rhs) = delete;
  Callbacks &operator=(Callbacks &&rhs)             = delete;
  bool       operator==(const Callbacks &rhs) const = delete;
  bool       operator<(const Callbacks &rhs) const  = delete;

  Callbacks &operator=(FUNC func)
  {
    callbacks_.push_back(func);
    return *this;
  }

  operator bool() const
  {
    return !callbacks_.empty();
  }

  template <class... U>
  void operator()(U &&... u)
  {
    for (auto func : callbacks_)
    {
      func(std::forward<U>(u)...);
    }
  }

  void clear(void)
  {
    callbacks_.clear();
  }

  explicit Callbacks()
  {}

  virtual ~Callbacks()
  {}

private:
  std::vector<FUNC> callbacks_;
};

}  // namespace generics
}  // namespace fetch
