#pragma once
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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

template <class Container>
void split(const std::string &str, Container &cont, char delim = ' ')
{
  std::stringstream ss(str);
  std::string       token;
  while (std::getline(ss, token, delim))
  {
    cont.push_back(token);
  }
  if (str.back() == '/')
  {
    cont.push_back("");
  }
}

namespace OEFURI {

class URI
{
public:
  std::string              protocol;
  std::string              coreURI;
  std::string              CoreKey;
  std::vector<std::string> namespaces;
  std::string              AgentKey;
  std::string              AgentAlias;
  bool                     empty;

  URI()
    : protocol{"tcp"}
    , coreURI{""}
    , CoreKey{""}
    , AgentKey{""}
    , AgentAlias{""}
    , empty{true}
  {}

  virtual ~URI() = default;

  std::string ToString() const
  {
    std::string nspaces;
    if (!namespaces.empty())
    {
      for (const std::string &nspace : namespaces)
      {
        nspaces += nspace + "/";
      }
      nspaces = nspaces.substr(0, nspaces.size() - 1);
    }
    std::string uri;
    uri += protocol + "://" += coreURI + "/" += CoreKey + "/" += nspaces + "/" += AgentKey + "/" +=
        AgentAlias;
    return uri;
  }

  std::string AgentPartAsString()
  {
    return AgentKey + (!AgentAlias.empty() ? ("/" + AgentAlias) : "");
  }

  void parse(const std::string &uri)
  {
    std::vector<std::string> vec;
    split(uri, vec, '/');
    if (vec.size() < 7)
    {
      return;
    }
    std::cerr << "GOT: " << uri << std::endl;
    std::cerr << "SIZE: " << vec.size() << std::endl;
    for (std::size_t i = 0; i < vec.size(); ++i)
    {
      std::cerr << i << ": " << vec[i] << std::endl;
    }
    empty      = false;
    protocol   = vec[0].substr(0, vec[0].size() - 1);
    coreURI    = vec[2];
    CoreKey    = vec[3];
    AgentAlias = vec[vec.size() - 1];
    AgentKey   = vec[vec.size() - 2];
    for (std::size_t i = 4, limit = vec.size() - 2; i < limit; ++i)
    {
      namespaces.push_back(vec[i]);
    }
  }

  void ParseAgent(const std::string &src)
  {
    if (src.find('/') == std::string::npos)
    {
      AgentKey = src;
      return;
    }
    std::vector<std::string> vec;
    split(src, vec, '/');
    if (vec.size() != 2)
    {
      std::cerr << "parseDestination got invalid string: " << src
                << "! Splitted size: " << vec.size() << std::endl;
      return;
    }
    empty      = false;
    AgentKey   = vec[0];
    AgentAlias = vec[1];
  }

  void print()
  {
    std::cout << "protocol: " << protocol << std::endl
              << "coreURI: " << coreURI << std::endl
              << "CoreKey: " << CoreKey << std::endl
              << "AgentKey: " << AgentKey << std::endl
              << "AgentAlias: " << AgentAlias << std::endl
              << "empty: " << empty << std::endl
              << "namespaces: " << std::endl;
    for (const auto &n : namespaces)
    {
      std::cout << "    - " << n << std::endl;
    }
  }
};

class Builder
{
public:
  using BuilderPtr = std::shared_ptr<Builder>;

  static BuilderPtr create(URI const &uri = URI())
  {
    return std::make_shared<Builder>(uri);
  }

  explicit Builder(URI const &uri)
    : uri_(uri)
  {}

  virtual ~Builder() = default;

  Builder *protocol(std::string protocol)
  {
    uri_.protocol = std::move(protocol);
    return this;
  }

  Builder *CoreAddress(std::string host, int port)
  {
    uri_.coreURI = std::move(host);
    uri_.coreURI += std::to_string(port);
    return this;
  }

  Builder *CoreKey(std::string key)
  {
    uri_.CoreKey = std::move(key);
    return this;
  }

  Builder *AgentKey(std::string key)
  {
    uri_.AgentKey = std::move(key);
    return this;
  }

  Builder *AddNamespace(std::string nspace)
  {
    uri_.namespaces.emplace_back(std::move(nspace));
    return this;
  }

  Builder *AgentAlias(std::string alias)
  {
    uri_.AgentAlias = std::move(alias);
    return this;
  }

  URI build()
  {
    return uri_;
  }

private:
  URI uri_;
};
}  // namespace OEFURI
