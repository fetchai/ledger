#pragma once

#include <typeindex>
#include <vector>

namespace fetch {
namespace semanticsearch {

namespace details {
template <typename R, typename A, typename... Args>
struct ArgumentsToSemanticPosition
{
  static void Apply(std::vector<std::type_index> &args)
  {
    args.push_back(std::type_index(typeid(A)));
    ArgumentsToSemanticPosition<R, Args...>::Apply(args);
  }
};

template <typename R, typename A>
struct ArgumentsToSemanticPosition<R, A>
{
  static void Apply(std::vector<std::type_index> &args)
  {
    args.push_back(std::type_index(typeid(A)));
  }
};

template <typename R>
struct ArgumentsToSemanticPosition<R, void>
{
  static void Apply(std::vector<std::type_index> & /*args*/)
  {}
};

template <uint64_t N, typename... UsedArgs>
struct SemanticPositionToArguments
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
      SemanticPositionToArguments<N - 1, UsedArgs..., B>::template Unroll<
          R, RemainingArguments...>::Apply(i + 1, std::move(fnc), return_value, data, args..., val);
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