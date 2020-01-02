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

#include <functional>
#include <typeindex>
#include <vector>

namespace fetch {
namespace semanticsearch {

namespace details {
/* This code converts C++ arguments into a
 * vector of types
 */
template <typename R, typename A, typename... Args>
struct ArgumentsToTypeVector
{
  static void Apply(std::vector<std::type_index> &args)
  {
    args.push_back(std::type_index(typeid(A)));
    ArgumentsToTypeVector<R, Args...>::Apply(args);
  }
};

template <typename R, typename A>
struct ArgumentsToTypeVector<R, A>
{
  static void Apply(std::vector<std::type_index> &args)
  {
    args.push_back(std::type_index(typeid(A)));
  }
};

template <typename R>
struct ArgumentsToTypeVector<R, void>
{
  static void Apply(std::vector<std::type_index> & /*args*/)
  {}
};

/* This code converts  a vector of type-erased types
 * into the types matching the argument types.
 */
template <uint64_t N, typename... UsedArgs>
struct VectorToArguments
{

  template <typename R, typename A, typename... RemainingArguments>
  struct Unroll
  {
    using CallerSignature = std::function<R(UsedArgs..., A, RemainingArguments...)>;

    static void Apply(std::size_t i, CallerSignature fnc, R &return_value,
                      std::vector<void const *> &data, UsedArgs &... args)
    {
      using B = typename std::decay<A>::type;
      B val   = *reinterpret_cast<B const *>(data[i]);
      VectorToArguments<N - 1, UsedArgs..., B>::template Unroll<R, RemainingArguments...>::Apply(
          i + 1, std::move(fnc), return_value, data, args..., val);
    }
  };

  template <typename R, typename A>
  struct Unroll<R, A>
  {
    using CallerSignature = std::function<R(UsedArgs..., A)>;

    static void Apply(std::size_t i, CallerSignature fnc, R &return_value,
                      std::vector<void const *> &data, UsedArgs &... args)
    {
      using B      = typename std::decay<A>::type;
      B val        = *reinterpret_cast<B const *>(data[i]);
      return_value = fnc(args..., val);
    }
  };
};

}  // namespace details

}  // namespace semanticsearch
}  // namespace fetch
