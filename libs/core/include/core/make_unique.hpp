#ifndef FETCH_MAKE_SHARED_HPP
#define FETCH_MAKE_SHARED_HPP

#include <memory>

namespace fetch {

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace fetch

#endif  // FETCH_MAKE_SHARED_HPP
