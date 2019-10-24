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

#include "semanticsearch/module/args_resolver.hpp"
#include "semanticsearch/query/variant.hpp"

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
    Function ret = Function(new BuiltinQueryFunction(std::type_index(typeid(R))));
    details::ArgumentsToSemanticPosition<R, Args...>::Apply(ret->arguments_);

    ret->caller_ = [caller](std::vector<void const *> &args) -> QueryVariant {
      R return_value;
      details::SemanticPositionToArguments<sizeof...(Args)>::template Unroll<R, Args...>::Apply(
          0, caller, return_value, args);
      return NewQueryVariant(return_value);
    };

    return ret;
  }

  bool ValidateSignature(std::type_index const &ret, std::vector<std::type_index> const &args);
  QueryVariant    operator()(std::vector<void const *> &args);
  std::type_index return_type() const;

private:
  BuiltinQueryFunction(std::type_index return_type)
    : return_type_(std::move(return_type))
  {}

  std::vector<std::type_index> arguments_;
  std::type_index              return_type_;

  CallerSignature caller_;
};

}  // namespace semanticsearch
}  // namespace fetch
