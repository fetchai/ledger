#pragma once
#include <type_traits>

namespace fetch
{
namespace meta
{
namespace detail
{

template <typename T>
auto IsIterableImplementation(int)
-> decltype(
      std::begin(std::declval<T&>()) != std::end(std::declval<T&>()),
      ++std::declval<decltype(std::begin(std::declval<T&>()))&>(), 
      *std::begin(std::declval<T&>()), 
      std::true_type{}
    );

template <typename T>
std::false_type IsIterableImplementation(...); 
}
 
template <typename T>
using IsIterable = std::enable_if<decltype(detail::IsIterableImplementation<T>(0)), T>::type;
  
}
}