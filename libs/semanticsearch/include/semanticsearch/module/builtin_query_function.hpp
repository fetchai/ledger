#pragma once

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
