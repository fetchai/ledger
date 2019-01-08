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

#include <utility>

namespace fetch {
namespace serializers {

template <typename T>
class LazyEvalArgument
{
  T val_;

public:
  LazyEvalArgument(T val)
    : val_{std::move(val)} {};

  LazyEvalArgument(LazyEvalArgument const &) = default;
  LazyEvalArgument(LazyEvalArgument &&)      = default;

  LazyEvalArgument &operator=(LazyEvalArgument const &) = default;
  LazyEvalArgument &operator=(LazyEvalArgument &&) = default;

  template <typename... ARGS>
  auto operator()(ARGS... args) const
  {
    auto const &val = val_;
    return val(args...);
  }

  template <typename... ARGS>
  auto operator()(ARGS... args)
  {
    return val_(args...);
  }

  operator T const &() const
  {
    return val_;
  }

  operator T &()
  {
    return val_;
  }
};

template <typename T>
auto LazyEvalArgumentFactory(T &&functor)
{
  return LazyEvalArgument<T>{std::forward<T>(functor)};
}

template <typename STREAM, typename T>
void Serialize(STREAM &stream, LazyEvalArgument<T> const &lazyEvalArgument)
{
  lazyEvalArgument(stream);
}

}  // namespace serializers
}  // namespace fetch
