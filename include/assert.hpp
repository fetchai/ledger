#ifndef ASSERT_HPP
#define ASSERT_HPP

#include <iostream>
namespace fetch {
namespace assert {
namespace details {
struct Printer {
  template <typename T, typename... Args>
  static void Print(T const &next, Args... args) {
    std::cerr << next;
    Print(args...);
  }

  template <typename T>
  static void Print(T const &next) {
    std::cerr << next;
  }
};
};
};
};


#ifndef FETCH_DISABLE_TODO_COUT
// TODO: Rename to FETCH_...
#define TODO_FAIL(...)                                                        \
  fetch::assert::details::Printer::Print(__VA_ARGS__);                        \
  std::cerr << std::endl << __FILE__ << " at line " << __LINE__ << std::endl; \
  exit(-1)

#define TODO(...)                                                        \
  fetch::assert::details::Printer::Print(__VA_ARGS__);                        \
  std::cerr << std::endl << __FILE__ << " at line " << __LINE__ << std::endl; 

#else

#define TODO_FAIL(...)                                                        \
  exit(-1)
#define TODO(...)                                                        

#endif

#define detailed_assert(cond) \
  if( !(cond) ) { \
    std::cout << "Failed :" << #cond << " in " << __FILE__ << " line " << __LINE__ << std::endl; \
    exit(-1); \
  }


#endif
