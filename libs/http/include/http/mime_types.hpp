#pragma once
#include <algorithm>
#include <string>
#include <vector>
namespace fetch {
namespace http {

struct MimeType
{
  std::string extension;
  std::string type;
  bool        operator<(MimeType const &other) const
  {
    return extension < other.extension;
  }
};

namespace mime_types {
MimeType GetMimeTypeFromExtension(std::string const &ext);
}
}  // namespace http
}  // namespace fetch
