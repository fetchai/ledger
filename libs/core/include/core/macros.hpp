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

#define FETCH_UNUSED(x) (void)(x)

template <typename>
struct Unused
{
  constexpr static void noop()
  {}
};
#define FETCH_UNUSED_ALIAS(x) (void)Unused<x>::noop()

#define FETCH_MAYBE_UNUSED __attribute__((used))

#if defined(__clang__)
#define FETCH_THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define FETCH_THREAD_ANNOTATION_ATTRIBUTE__(x)  // no-op
#endif

#define FETCH_GUARDED_BY(x) FETCH_THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define FETCH_PTR_GUARDED_BY(x) FETCH_THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))
