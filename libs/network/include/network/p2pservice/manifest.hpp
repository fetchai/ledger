#pragma once

#include <memory>

namespace fetch {

namespace network {


class Manifest
{
public:
  using ServiceIdentifier = int; // +ve numbers are lanes, -ve are other services.
  using Uri = std::string;

  enum {
    SERVICE_IDENTIFIER_MAIN_CHAIN = -2,
    SERVICE_IDENTIFIER_P2P = -1,
    SERVICE_IDENTIFIER_LANE0 = 0,
    SERVICE_IDENTIFIER_LANE1 = 1,
    SERVICE_IDENTIFIER_LANE2 = 2,
    SERVICE_IDENTIFIER_LANE3 = 3,
    // Do the rest with maths...
  };

  static Manifest fromMap( std::map<ServiceIdentifier, Uri> manifestData )
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

  Manifest& operator=(const Manifest& other)
  {
    if (&other != this)
    {
      data_ = other.data_;
    }
    return *this;
  }
private:
  std::map<ServiceIdentifier, Uri> data_;

  Manifest()
  {
  }
};

}
}
