#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/logger.hpp"
#include "core/serializers/stl_types.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"

#include <map>
#include <memory>
#include <vector>

namespace fetch {

namespace network {

class Manifest
{
public:
  using Uri               = network::Uri;
  using ServiceType       = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;

  static constexpr char const *LOGGING_NAME = "Manifest";

  static Manifest FromText(const std::string &inputlines)
  {
    std::map<ServiceIdentifier, Uri> temp;

    std::vector<std::string> strings;
    std::istringstream       f(inputlines);
    std::string              s;
    int                      linenum = 0;
    while (getline(f, s, '\n'))
    {
      linenum++;
      if (s.length() > 0)
      {
        if (s[0] != '#')
        {
          try
          {
            auto r = ParseLine(s);

            ServiceIdentifier id{std::get<0>(r), std::get<1>(r)};
            temp[id] = Uri{std::get<2>(r)};
          }
          catch (std::exception &ex)
          {
            throw std::invalid_argument(std::string(ex.what()) + " at line " +
                                        std::to_string(linenum));
          }
        }
      }
    }
    return FromMap(temp);
  }

  static Manifest FromMap(std::map<ServiceIdentifier, Uri> manifestData)
  {
    Manifest m;
    for (auto &item : manifestData)
    {
      m.data_[item.first] = item.second;
    }
    return m;
  }

  const std::map<ServiceIdentifier, Uri> &GetData()
  {
    return data_;
  }

  network::Uri GetUri(ServiceIdentifier service_id)
  {
    network::Uri uri;

    auto res = data_.find(service_id);
    if (res != data_.end())
    {
      uri = res->second;
    }

    return uri;
  }

  bool ContainsService(ServiceIdentifier service_id)
  {
    auto res = data_.find(service_id);
    return (res != data_.end());
  }

  Manifest()                      = default;
  Manifest(Manifest const &other) = default;
  Manifest(Manifest &&other)      = default;
  ~Manifest()                     = default;

  Manifest &operator=(Manifest &&other) = default;
  Manifest &operator=(Manifest const &other) = default;

  size_t size() const
  {
    return data_.size();
  }

  std::map<ServiceIdentifier, Uri>::const_iterator begin() const
  {
    return data_.begin();
  }
  std::map<ServiceIdentifier, Uri>::const_iterator end() const
  {
    return data_.end();
  }

  void ForEach(std::function<void(const ServiceIdentifier &, const Uri &)> cb) const
  {
    for (auto &i : data_)
    {
      cb(i.first, i.second);
    }
  }

  std::string ToString() const
  {
    std::string result;
    for (auto &data : data_)
    {
      std::string line = std::string(service_type_names[data.first.service_type]) + "  " +
                         std::to_string(data.first.instance_number) + "  " +
                         static_cast<std::string>(data.second.uri()) + "\n";
      result += line;
    }
    return result;
  }

private:
  std::map<ServiceIdentifier, Uri> data_;

  static const char *service_type_names[];

  static std::tuple<ServiceType, uint32_t, std::string> ParseLine(const std::string &str)
  {
    std::vector<std::string> store;
    store.push_back("");
    store.push_back("");
    store.push_back("");

    int  part   = -1;
    bool inword = false;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
    {

      if (part < 2)
      {
        if (*it == ' ' || *it == '\t')
        {
          inword = false;
          continue;
        }
      }
      if (!inword)
      {
        part++;
        FETCH_LOG_DEBUG(LOGGING_NAME, "PART=", part);
        inword = true;
      }
      store[uint32_t(part)] += *it;
    }

    auto        uri = store[2];
    ServiceType service_type;
    uint32_t    instance;

    if (store[0] == "MAINCHAIN")
    {
      service_type = ServiceType::MAINCHAIN;
      instance     = 0;
    }
    else if (store[0] == "P2P")
    {
      service_type = ServiceType::P2P;
      instance     = 0;
    }
    else if (store[0] == "HTTP")
    {
      service_type = ServiceType::HTTP;
      instance     = 0;
    }
    else if (store[0] == "LANE")
    {
      service_type = ServiceType::LANE;
      instance     = uint32_t(std::stoi(store[1].c_str()));
    }
    else
    {
      throw std::invalid_argument("'" + store[0] + "'");
    }

    return std::tuple<ServiceType, uint32_t, std::string>(service_type, instance, uri);
  }

  template <typename T>
  friend void Serialize(T &serializer, Manifest const &x)
  {
    serializer << x.data_;
  }

  template <typename T>
  friend void Deserialize(T &serializer, Manifest &x)
  {
    serializer >> x.data_;
  }
};

}  // namespace network
}  // namespace fetch
