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


#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <memory>
#include <iostream>

template <class Container>
void split(const std::string& str, Container& cont, char delim = ' ')
{
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delim)) {
    cont.push_back(token);
  }
  if (str.back() == '/'){
    cont.push_back("");
  }
}

namespace OEFURI {

  class URI {
  public:
    std::string protocol;
    std::string coreURI;
    std::string coreKey;
    std::vector<std::string> namespaces;
    std::string agentKey;
    std::string agentAlias;
    bool empty;

    URI()
        : protocol{"tcp"}, coreURI{""}, coreKey{""}, agentKey{""}, agentAlias{""}, empty{true}
    {
    }

    virtual ~URI() = default;


    std::string toString() {
      std::string nspaces = "";
      if (!namespaces.empty()) {
        for (const std::string &nspace : namespaces) {
          nspaces += nspace + "/";
        }
        nspaces = nspaces.substr(0, nspaces.size() - 1);
      }
      std::string uri{""};
      uri += protocol + "://"
          += coreURI + "/"
          += coreKey + "/"
          += nspaces + "/"
          += agentKey + "/"
          += agentAlias;
      return uri;
    }

    std::string agentPartAsString()
    {
      return agentKey + (agentAlias.size()>0 ? ("/" + agentAlias) : "");
    }

    void parse(const std::string &uri) {
      std::vector<std::string> vec;
      split(uri, vec, '/');
      if (vec.size() < 7) {
        return;
      }
      std::cerr << "GOT: " << uri << std::endl;
      std::cerr << "SIZE: " << vec.size() << std::endl;
      for(int i=0;i<vec.size();++i) {
        std::cerr << i << ": " << vec[i] << std::endl;
      }
      empty = false;
      protocol = vec[0].substr(0, vec[0].size()-1);
      coreURI = vec[2];
      coreKey = vec[3];
      agentAlias = vec[vec.size() - 1];
      agentKey = vec[vec.size() - 2];
      for (std::size_t i = 4, limit = vec.size() - 2; i < limit; ++i) {
        namespaces.push_back(vec[i]);
      }
    }

    void parseAgent(const std::string& src)
    {
      if (src.find('/')==std::string::npos){
        agentKey = src;
        return;
      }
      std::vector<std::string> vec;
      split(src, vec,'/');
      if (vec.size()!=2) {
        std::cerr << "parseDestination got invalid string: " << src << "! Splitted size: " << vec.size() << std::endl;
        return;
      }
      empty = false;
      agentKey = vec[0];
      agentAlias = vec[1];
    }

    void print() {
      std::cout << "protocol: " << protocol << std::endl
                << "coreURI: " << coreURI << std::endl
                << "coreKey: " << coreKey << std::endl
                << "agentKey: " << agentKey << std::endl
                << "agentAlias: " << agentAlias << std::endl
                << "empty: " << empty << std::endl
                << "namespaces: " << std::endl;
      for (const auto &n : namespaces) {
        std::cout << "    - " << n << std::endl;
      }
    }
  };

  class Builder {
  public:
    using BuilderPtr = std::shared_ptr<Builder>;

    static BuilderPtr create(URI uri = URI()) {
      return std::make_shared<Builder>(std::move(uri));
    }

    Builder(URI uri)
        : uri_(std::move(uri))
    {
    }

    virtual ~Builder() = default;

    Builder *protocol(std::string protocol) {
      uri_.protocol = std::move(protocol);
      return this;
    }

    Builder *coreAddress(std::string host, int port) {
      uri_.coreURI = std::move(host);
      uri_.coreURI += std::to_string(port);
      return this;
    }

    Builder *coreKey(std::string key) {
      uri_.coreKey = std::move(key);
      return this;
    }

    Builder *agentKey(std::string key) {
      uri_.agentKey = std::move(key);
      return this;
    }

    Builder *addNamespace(std::string nspace) {
      uri_.namespaces.push_back(std::move(nspace));
      return this;
    }

    Builder *agentAlias(std::string alias) {
      uri_.agentAlias = std::move(alias);
      return this;
    }

    URI build() {
      return std::move(uri_);
    }


  private:
    URI uri_;
  };
}