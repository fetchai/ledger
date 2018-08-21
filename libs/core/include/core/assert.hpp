#pragma once

#include <iostream>
#include <stdexcept>
namespace fetch {
namespace assert {
namespace details {
struct Printer
{
  template <typename T, typename... Args>
  static void Print(T const &next, Args... args)
  {
    std::cerr << next;
    Print(args...);
  }

  template <typename T>
  static void Print(T const &next)
  {
    std::cerr << next;
  }
};
}  // namespace details
}  // namespace assert
}  // namespace fetch

#ifndef FETCH_DISABLE_TODO_COUT
// TODO: Rename to FETCH_...
#define TODO_FAIL(...)                                                        \
  fetch::assert::details::Printer::Print(__VA_ARGS__);                        \
  fetch::logger.Error("About to fail.");                 \
  std::cerr << std::endl << __FILE__ << " at line " << __LINE__ << std::endl; \
  throw std::runtime_error("Dependence on non-existing functionality!");

#define TODO(...)                                      \
  fetch::assert::details::Printer::Print(__VA_ARGS__); \
  std::cerr << std::endl << __FILE__ << " at line " << __LINE__ << std::endl;

#else

#define TODO_FAIL(...) throw std::runtime_error("Dependence on non-existing functionality!");
#define TODO(...)

#endif

#define detailed_assert(cond)                                                                    \
  if (!(cond))                                                                                   \
  {                                                                                              \
    std::cout << "Failed :" << #cond << " in " << __FILE__ << " line " << __LINE__ << std::endl; \
    throw std::runtime_error("Assertion failed");                                                \
  }
