#ifndef FETCH_REPLACE_HPP
#define FETCH_REPLACE_HPP

#include <algorithm>

namespace fetch {
namespace string {

inline std::string Replace(std::string value, char before, char after)
{
  std::replace(value.begin(), value.end(), before, after);
  return value;
}

}  // namespace string
}  // namespace fetch

#endif  // FETCH_REPLACE_HPP
