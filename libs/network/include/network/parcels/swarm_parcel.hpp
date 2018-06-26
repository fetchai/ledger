#ifndef __SWARM_PARCEL__
#define __SWARM_PARCEL__

#include <iostream>
#include <openssl/md5.h>
#include <sstream>
#include <stdio.h>
#include <string>

namespace fetch
{
namespace swarm
{

class SwarmParcel
{
public:

  std::string name_, data_, type_;

  SwarmParcel(const std::string &type, const std::string &data)
  {
    type_ = type;
    data_ = data;

    unsigned char result[MD5_DIGEST_LENGTH];
    MD5((unsigned char*) data_.c_str(), data_.size(), result);
    char hexResult[MD5_DIGEST_LENGTH * 2 + 1];
    for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
      ::sprintf(hexResult+i*2,"%02x", result[i]);
    }

    name_ = hexResult;
  }

  SwarmParcel(const SwarmParcel &rhs)
  {
    type_ = rhs.type_;
    name_ = rhs.name_;
    data_ = rhs.data_;
  }

  SwarmParcel(SwarmParcel &&rhs)
  {
    type_ = std::move(rhs.type_);
    name_ = std::move(rhs.name_);
    data_ = std::move(rhs.data_);
  }

  SwarmParcel operator=(const SwarmParcel &rhs)
  {
    type_ = rhs.type_;
    name_ = rhs.name_;
    data_ = rhs.data_;
    return *this;
  }
  SwarmParcel operator=(SwarmParcel &&rhs)
  {
    type_ = std::move(rhs.type_);
    name_ = std::move(rhs.name_);
    data_ = std::move(rhs.data_);
    return *this;
  }

  bool operator==(const SwarmParcel &rhs) const
  {
    return name_ == rhs.name_;
  }

  virtual ~SwarmParcel()
  {
  }

  const std::string &GetType() const { return type_; }
  const std::string &GetData() const { return data_; }
  const std::string &GetName() const { return name_; }

  std::string asJSON() const
  {
    std::ostringstream ret;

    ret << "{" << std::endl;
    ret << "  \"name\": \"" << name_ << "\"," << std::endl;
    ret << "  \"type\": \"" << type_ << "\"," << std::endl;
    ret << "  \"data\": " << data_ << std::endl;
    ret << "}" << std::endl;

    return ret.str();
  }
};

}
}

#endif //__SWARM_PARCEL__
