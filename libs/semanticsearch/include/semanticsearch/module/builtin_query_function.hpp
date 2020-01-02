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

#include "semanticsearch/module/args_resolver.hpp"
#include "semanticsearch/query/abstract_query_variant.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace fetch {
namespace semanticsearch {

class BuiltinQueryFunction
{
public:
  using CallerSignature = std::function<QueryVariant(std::vector<void const *> &)>;
  using Function        = std::shared_ptr<BuiltinQueryFunction>;

  template <typename R, typename... Args>
  static Function New(std::function<R(Args...)> caller)
  {
    // Creating a shared builtin query function and
    // computes a list of type indices for the function arguments
    Function ret = Function(new BuiltinQueryFunction(std::type_index(typeid(R))));
    details::ArgumentsToTypeVector<R, Args...>::Apply(ret->arguments_);

    // Creating a type converter which converts type-erased void* arguments
    // into their original types
    ret->caller_ = [caller](std::vector<void const *> &args) -> QueryVariant {
      R return_value;

      // Calls the original caller function
      details::VectorToArguments<sizeof...(Args)>::template Unroll<R, Args...>::Apply(
          0, caller, return_value, args);

      // Creating a resulting variant for the return type.
      return NewQueryVariant(return_value);
    };

    return ret;
  }

  bool ValidateSignature(std::type_index const &ret, std::vector<std::type_index> const &args);
  QueryVariant    operator()(std::vector<void const *> &args);
  std::type_index return_type() const;

private:
  explicit BuiltinQueryFunction(std::type_index return_type)
    : return_type_(return_type)
  {}

  std::vector<std::type_index> arguments_;
  std::type_index              return_type_;

  CallerSignature caller_;
};

}  // namespace semanticsearch
}  // namespace fetch
