#ifndef FETCH_REPLACE_HPP
#define FETCH_REPLACE_HPP

namespace fetch {
namespace string {

inline std::string Replace(std::string value, char before, char after) {
  for (char &c : value) {
    if (c == before) {
      c = after;
    }
  }
  return value;
}

} // namespace string
} // namespace fetch

#endif //FETCH_REPLACE_HPP
