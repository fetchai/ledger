#pragma once
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

#include <chrono>
#include <string>
#include <unordered_map>

#include "dmlf/colearn/colearn_update.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class ColearnURI
{
public:
  ColearnURI() = default;
  explicit ColearnURI(ColearnUpdate const &update);

  std::string       ToString() const;
  static ColearnURI Parse(std::string const &uriString);  // Fingerprint must be Base58

  bool IsEmpty() const;

  std::string protocol() const
  {
    return protocol_;
  }
  std::string owner() const
  {
    return owner_;
  }
  ColearnURI &owner(std::string name)
  {
    owner_ = std::move(name);
    return *this;
  }
  std::string algorithm_class() const
  {
    return algorithm_class_;
  }
  ColearnURI &algorithm_class(std::string name)
  {
    algorithm_class_ = std::move(name);
    return *this;
  }
  std::string update_type() const
  {
    return update_type_;
  }
  ColearnURI &update_type(std::string name)
  {
    update_type_ = std::move(name);
    return *this;
  }
  std::string source() const
  {
    return source_;
  }
  ColearnURI &source(std::string name)
  {
    source_ = std::move(name);
    return *this;
  }
  std::string fingerprint() const
  {
    return fingerprint_;
  }
  ColearnURI &fingerprint(std::string fingerprint)
  {
    fingerprint_ = std::move(fingerprint);
    return *this;
  }

private:
  std::string protocol_ = "colearn";
  std::string owner_;
  std::string algorithm_class_;
  std::string update_type_;
  std::string source_;
  std::string fingerprint_;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
