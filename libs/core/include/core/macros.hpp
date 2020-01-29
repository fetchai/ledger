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

namespace fetch {

template <class = void>
struct Unused
{
  template <class... Args>
  constexpr static void noop(Args &&... /*unused*/) noexcept
  {}
};

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#define FETCH_UNUSED(...) ::fetch::Unused<>::noop(__VA_ARGS__)

#define FETCH_UNUSED_IN_NAMESPACE(name)                                                     \
  inline auto FETCH_JOIN(name, _FinallyUsed_AndHopefullyThisNameWillNotClashWithAnything)() \
  {                                                                                         \
    return name;                                                                            \
  }

#define FETCH_UNUSED_ALIAS(x) (void)Unused<x>::noop()

#define FETCH_MAYBE_UNUSED __attribute__((used))

#if defined(__clang__)
#define FETCH_THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define FETCH_THREAD_ANNOTATION_ATTRIBUTE__(x)  // no-op
#endif

#define FETCH_GUARDED_BY(x) FETCH_THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define FETCH_PTR_GUARDED_BY(x) FETCH_THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

// [[fallthrough]] is not supported in C++14
#if (__cplusplus >= 201703L)
#define FETCH_FALLTHROUGH [[fallthrough]]
#else
#if defined(__clang__)
#define FETCH_FALLTHROUGH [[clang::fallthrough]]
#elif defined(__GNUC__)
#define FETCH_FALLTHROUGH [[gnu::fallthrough]]
#endif
#endif

}  // namespace fetch
