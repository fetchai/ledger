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

#include <type_traits>
#include <utility>

namespace fetch {
namespace meta {

namespace internal {

class free_or_static_member_fn_tag;
class member_fn_tag;
class const_member_fn_tag;
class functor_tag;

template <typename Callable>
struct ClassifyCallableImpl
{
private:
  template <typename R, typename... A>
  static free_or_static_member_fn_tag GetCallableTag(R (*&&)(A...));
  template <typename R, typename T, typename... A>
  static member_fn_tag GetCallableTag(R (T::*&&)(A...));
  template <typename R, typename T, typename... A>
  static const_member_fn_tag GetCallableTag(R (T::*&&)(A...) const);
  template <typename F>
  static functor_tag GetCallableTag(F &&);

public:
  using type = decltype(GetCallableTag(std::declval<std::decay_t<Callable>>()));
};

template <typename F>
using ClassifyCallable = typename ClassifyCallableImpl<F>::type;

}  // namespace internal

}  // namespace meta
}  // namespace fetch
