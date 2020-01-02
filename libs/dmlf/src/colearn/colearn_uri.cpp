//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "dmlf/colearn/colearn_uri.hpp"

#include "core/byte_array/encoders.hpp"

#include <array>
#include <string>
#include <unordered_map>

namespace fetch {
namespace dmlf {
namespace colearn {

ColearnURI::ColearnURI(ColearnUpdate const &update)
  : algorithm_class_(update.algorithm())
  , update_type_(update.update_type())
  , source_(update.source())
  , fingerprint_(fetch::byte_array::ToBase58(update.fingerprint()))
{}

ColearnURI ColearnURI::Parse(std::string const &uriString)
{
  ColearnURI    result;
  constexpr int prefixLength = 10;
  if (uriString.compare(0, prefixLength, "colearn://") != 0)
  {
    return result;
  }

  auto current = uriString.cbegin() + prefixLength - 1;
  auto end     = uriString.cend();

  std::vector<std::string> fields;
  while (current != end)
  {
    ++current;
    auto nextSlash = std::find(current, end, '/');
    fields.emplace_back(current, nextSlash);
    current = nextSlash;
  }

  if (fields.size() != 5)
  {
    return result;
  }

  result.owner(fields[0])
      .algorithm_class(fields[1])
      .update_type(fields[2])
      .source(fields[3])
      .fingerprint(fields[4]);

  return result;
}

std::string ColearnURI::ToString() const
{
  return protocol() + "://" + owner() + '/' + algorithm_class() + '/' + update_type() + '/' +
         source() + '/' + fingerprint();
}

bool ColearnURI::IsEmpty() const
{
  return owner_.empty() && algorithm_class_.empty() && update_type_.empty() && source_.empty() &&
         fingerprint_.empty();
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
