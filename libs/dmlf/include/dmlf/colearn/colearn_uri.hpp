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

#include "dmlf/colearn/update_store.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class ColearnURI
{
public:
  ColearnURI() = default;
  ColearnURI(UpdateStore::Update const & update)
  : algorithm_class_(update.algorithm())
  , update_type_(update.update_type())
  , source_(update.source())
  , fingerprint_(update.fingerprint().ToBase64())
  {
    EncodeFingerprint(fingerprint_);
  }

  ColearnURI(std::string const& uriString)
  {

    auto prefixEnd = uriString.find("://");
    if (prefixEnd == std::string::npos)
    {
      return;
    }

    


    if (!hasProtocol(uriString))
    {
      return;
    }

    auto count = std::count(uriString.cbegin()+ prefixEnd+3, uriString.cend(), '/');
    if (count < 6)
    {
      return;
    }
    
    std::string::const_reverse_iterator begin = uriString.rbegin(); 
    std::string::const_reverse_iterator end   = uriString.rend() - 10; 

    fingerprint_     = Pop(begin,end);
    source_          = Pop(begin,end);
    update_type_     = Pop(begin,end);
    algorithm_class_ = Pop(begin,end);
    owner_           = Pop(begin,end);
  }

  std::string ToString() const
  {
    return protocol()     + "://" 
      + owner()           + '/' 
      + algorithm_class() + '/' 
      + update_type()     + '/' 
      + source()          + '/'
      + fingerprint();
  }

  bool IsEmpty() const
  {
    return owner_.empty() && algorithm_class_.empty() && update_type_.empty() && source_.empty()
      && fingerprint_.empty();
  }

  std::string protocol() const
  {
    return protocol_;
  }
  std::string owner() const
  {
    return owner_;
  }
  ColearnURI& owner(std::string name)
  {
    owner_ = std::move(name);
    return *this;
  }
  std::string algorithm_class() const
  {
    return algorithm_class_;
  }
  ColearnURI& algorithm_class(std::string name)
  {
    algorithm_class_ = std::move(name);
    return *this;
  }
  std::string update_type() const
  {
    return update_type_;
  }
  ColearnURI& update_type(std::string name)
  {
    update_type_ = std::move(name);
    return *this;
  }
  std::string source() const
  {
    return source_;
  }
  ColearnURI& source(std::string name)
  {
    source_ = std::move(name);
    return *this;
  }
  std::string fingerprint() const
  {
    return fingerprint_;
  }
  std::string fingerprintAsBase64() const
  {
    return DecodeFingerprint(fingerprint_);
  }

  ColearnURI& fingerprint(std::string fingerprint)
  {
    fingerprint_ = std::move(fingerprint);
    return *this;
  }

private:
  bool hasProtocol(std::string const& uriString)
  {
    return uriString.find(protocol() + "://") == 0;
  }

  std::string EncodeFingerprint(std::string& fingerprint)
  {
    std::transform(fingerprint.begin(), fingerprint.end(), fingerprint.begin(), 
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
  }

  std::string Pop(std::string::const_reverse_iterator &begin, std::string::const_reverse_iterator end)
  {
    auto nextSlash = std::find(begin, end, '/');
    auto origBegin = begin;
    begin = nextSlash;
    return std::string(origBegin,nextSlash);
  }

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
