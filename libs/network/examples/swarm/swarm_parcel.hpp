#ifndef __SWARM_PARCEL__
#define __SWARM_PARCEL__

#include <iostream>
#include <string>

#include <openssl/md5.h>
#include <stdio.h>

using std::endl;
using std::string;

namespace fetch
{
namespace swarm
{

class SwarmParcel
{
public:

  std::string name_, data_, type_;

  SwarmParcel(const string &type, const string &data)
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

  const string &GetType() const { return type_; }
  const string &GetData() const { return data_; }
  const string &GetName() const { return name_; }

  std::string asJSON() const
  {
    std::ostringstream ret;

    ret << "{" << endl;
    ret << "  \"name\": \"" << name_ << "\"," << endl;
    ret << "  \"type\": \"" << type_ << "\"," << endl;
    ret << "  \"data\": " << data_ << endl;
    ret << "}" << endl;

    return ret.str();
  }
};

}
}

#endif //__SWARM_PARCEL__
