//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include <string>
#include <unordered_map>

namespace fetch {
namespace dmlf {
namespace colearn {

ColearnURI::ColearnURI(UpdateStore::Update const & update)
: algorithm_class_(update.algorithm())
, update_type_(update.update_type())
, source_(update.source())
//, fingerprint_(EncodeFingerprint(update.fingerprint().ToBase64()))
, fingerprint_(EncodeFingerprint("abc/+efg"))
{
}

  // Must be encoded
ColearnURI::ColearnURI(std::string const& uriString)
{
  constexpr int prefixLength = 10;
  if (uriString.compare(0, prefixLength, "colearn://") != 0)
  {
    return;
  }

  auto current = uriString.cbegin() + prefixLength - 1;
  auto end     = uriString.cend();

  std::vector<std::string> fields;
  while (current != end)
  {
    ++current;
    auto nextSlash = std::find(current, end, '/');
    fields.emplace_back(current,nextSlash);
    current = nextSlash;
  }

  if (fields.size() != 5)
  {
    return;
  }

  owner_ = fields[0];
  algorithm_class_ = fields[1];
  update_type_ = fields[2];
  source_ = fields[3];
  fingerprint_ = fields[4];
}

std::string ColearnURI::ToString() const
{
  return protocol()     + "://" 
    + owner()           + '/' 
    + algorithm_class() + '/' 
    + update_type()     + '/' 
    + source()          + '/'
    + fingerprint();
}

bool ColearnURI::IsEmpty() const
{
  return owner_.empty() && algorithm_class_.empty() && update_type_.empty() && source_.empty()
    && fingerprint_.empty();
}

std::string ColearnURI::EncodeFingerprint(std::string const &fingerprint)
{
  std::string result;
  std::transform(fingerprint.cbegin(), fingerprint.cend(), std::back_inserter(result), 
      [] (char c)
  {
    if (c == '+')
    {
      return '.';
    }
    if (c == '/')
    {
      return '_';
    }
    if (c == '=')
    {
      return '-';
    }
    return c;
  });
  return result;
}
std::string ColearnURI::DecodeFingerprint(std::string const &fingerprint)
{
  std::string result;
  std::transform(fingerprint.cbegin(), fingerprint.cend(), std::back_inserter(result),
      [] (char c)
  {
    if (c == '.')
    {
      return '+';
    }
    if (c == '_')
    {
      return '/';
    }
    if (c == '-')
    {
      return '=';
    }
    return c;
  });
  return result;
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
