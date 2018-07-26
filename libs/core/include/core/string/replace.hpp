#pragma once

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
