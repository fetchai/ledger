#ifndef FETCH_TO_LOWER_HPP
#define FETCH_TO_LOWER_HPP

#include <string>
#include <cstring>
#include <algorithm>

namespace fetch {
namespace string {

inline void ToLower(std::string &text)
{
  std::transform(
    text.begin(),
    text.end(),
    text.begin(),
    [](char c) -> char
    {
      return static_cast<char>(std::tolower(c));
    }
  );
}

} // namespace string
} // namespace fetch

#endif //FETCH_TO_LOWER_HPP
