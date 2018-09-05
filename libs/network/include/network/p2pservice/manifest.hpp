#pragma once

#include "core/logger.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"

#include <memory>
#include <map>
#include <vector>

namespace fetch {

namespace network {

class Manifest
{
public:

  using Uri = network::Uri;
  using ServiceType = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;

  static constexpr char const *LOGGING_NAME = "Manifest";

  static Manifest FromText(const std::string &inputlines)
  {
    std::map<ServiceIdentifier, Uri> temp;

    std::vector<std::string> strings;
    std::istringstream f(inputlines);
    std::string s;
    int linenum = 0;
    while (getline(f, s, '\n')) {
      linenum++;
      if (s.length() > 0)
      {
        if (s[0] != '#')
        {
          try
          {
            auto r = ParseLine(s);
            temp[{std::get<0>(r), std::get<0>(r)}] = std::get<2>(r);
          }
          catch (std::exception &ex)
          {
            throw std::invalid_argument(std::string(ex.what()) + " at line " + std::to_string(linenum));
          }
        }
      }
    }
    return FromMap(temp);
  }

  static Manifest FromMap( std::map<ServiceIdentifier, Uri> manifestData )
  {
    Manifest m;
    for(auto &item : manifestData)
    {
      m.data_[item.first] = item.second;
    }
    return m;
  }

  const std::map<ServiceIdentifier, Uri> &GetData()
  {
    return data_;
  }

  std::string GetUri(ServiceIdentifier service_id)
  {
    auto res = data_.find(service_id);
    if (res == data_.end())
    {
      return "";
    }
    else
    {
      return res -> second;
    }
  }

  bool ContainsService(ServiceIdentifier service_id)
  {
    auto res = data_.find(service_id);
    return (res != data_.end());
  }

  ~Manifest() = default;

  Manifest(const Manifest &other)
    : data_(other.data_)
  {
  }

  Manifest(Manifest &&other)
    : data_(std::move(other.data_))
  {
  }

  Manifest& operator=(Manifest&& other)
  {
    if (&other != this)
    {
      data_ = std::move(other.data_);
    }
    return *this;
  }

  std::map<ServiceIdentifier, Uri>::const_iterator begin() const
  {
    return data_.begin();
  }
  std::map<ServiceIdentifier, Uri>::const_iterator end() const
  {
    return data_.end();
  }

  Manifest& operator=(const Manifest& other)
  {
    if (&other != this)
    {
      data_ = other.data_;
    }
    return *this;
  }

  Manifest()
  {
  }
private:
  std::map<ServiceIdentifier, Uri> data_;

  static std::tuple<ServiceType, uint32_t, std::string> ParseLine(const std::string &str)
  {
    std::vector<std::string> store;
    store.push_back("");
    store.push_back("");
    store.push_back("");

    int part = -1;
    bool inword = false;
    for(std::string::const_iterator it = str.begin(); it != str.end(); ++it) {

      if (part<2)
      {
        if (*it == ' ' ||*it == '\t')
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

    auto uri = store[2];
    ServiceType service_type;
    uint32_t instance;

    if (store[0] == "MAINCHAIN")
    {
      service_type = ServiceType::MAINCHAIN;
      instance = 0;
    }
    else if (store[0] == "LANE")
    {
      service_type = ServiceType::LANE;
      instance = uint32_t(std::stoi(store[1].c_str()));
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
    std::vector<std::pair<ServiceIdentifier, Uri>> elementlist;
    for(auto item : x.data_)
    {
      elementlist.push_back(item);
    }
    serializer << elementlist;
  }

  template <typename T>
  friend void Deserialize(T &serializer, Manifest &x)
  {
    std::vector<std::pair<ServiceIdentifier, Uri>> elementlist;
    serializer >> elementlist;

    for(auto element: elementlist)
    {
      x.data_[element.first] = element.second;
    }
  }
};

}
}
