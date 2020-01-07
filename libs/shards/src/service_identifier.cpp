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

#include "shards/service_identifier.hpp"

#include <sstream>

namespace fetch {
namespace shards {

char const *ToString(ServiceIdentifier::Type type)
{
  char const *text = "Unknown";

  switch (type)
  {
  case ServiceIdentifier::Type::INVALID:
    text = "Invalid";
    break;
  case ServiceIdentifier::Type::CORE:
    text = "Core";
    break;
  case ServiceIdentifier::Type::HTTP:
    text = "Http";
    break;
  case ServiceIdentifier::Type::DKG:
    text = "Dkg";
    break;
  case ServiceIdentifier::Type::AGENTS:
    text = "Agents";
    break;
  case ServiceIdentifier::Type::LANE:
    text = "Lane";
    break;
  }

  return text;
}

ServiceIdentifier::ServiceIdentifier(Type type, uint32_t instance)
  : type_{type}
  , instance_{instance}
{}

std::string ServiceIdentifier::ToString() const
{
  std::ostringstream oss;
  oss << shards::ToString(type_);

  if (instance_ != ServiceIdentifier::SINGLETON_SERVICE)
  {
    oss << '/' << instance_;
  }

  return oss.str();
}

bool ServiceIdentifier::operator==(ServiceIdentifier const &other) const
{
  return (type_ == other.type_) && (instance_ == other.instance_);
}

bool ServiceIdentifier::operator<(ServiceIdentifier const &other) const
{
  if (type_ == other.type_)
  {
    return instance_ < other.instance_;
  }

  return type_ < other.type_;
}

}  // namespace shards
}  // namespace fetch
