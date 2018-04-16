#ifndef NETWORK_CLASSES_HPP
#define NETWORK_CLASSES_HPP

namespace fetch
{
namespace network_test
{

class Endpoint {
public:
  Endpoint() {}

  Endpoint(const std::string &IP, const int TCPPort)      : IP_{IP}, TCPPort_{uint16_t(TCPPort)} {}
  Endpoint(const std::string &IP, const uint16_t TCPPort) : IP_{IP}, TCPPort_{TCPPort} {}

  //Endpoint(Endpoint const &rhs)            = default;
  //Endpoint(Endpoint &&rhs)                 = default;
  //Endpoint &operator=(Endpoint const &rhs) = default;
  //Endpoint &operator=(Endpoint&& rhs)      = default;

  Endpoint(const json::JSONDocument &jsonDoc)
  {
    LOG_STACK_TRACE_POINT;

    IP_      = std::string(jsonDoc["IP"].as_byte_array());

    // TODO: (`HUT`) : fix after this parsing works
    if (jsonDoc["TCPPort"].is_int())
    {
      TCPPort_ = uint16_t(jsonDoc["TCPPort"].as_int());
    }
    else if (jsonDoc["TCPPort"].is_float())
    {
      float value = static_cast<float>(jsonDoc["TCPPort"].as_double());
      TCPPort_ = uint16_t(value);
    }
    else
    {
      TCPPort_ = 0;
    }

    std::cout << "jsondoc: " << IP_ << ":" << TCPPort_ << std::endl;
  }

  bool operator< (const Endpoint &rhs) const
  {
      return (TCPPort_ < rhs.TCPPort()) || (IP_ < rhs.IP());
  }

  bool equals(const Endpoint &rhs) const
  {
    return (TCPPort_ == rhs.TCPPort()) && (IP_ == rhs.IP());
  }

  script::Variant variant() const
  {
    script::Variant result = script::Variant::Object();
    result["IP"]      = IP_;
    result["TCPPort"] = TCPPort_;
    std::cout << "variant: " << IP_ << ":" << TCPPort_ << std::endl;
    return result;
  }

  const std::string &IP() const      { return IP_; }
  std::string       &IP()            { return IP_; }
  const uint16_t    &TCPPort() const { return TCPPort_; }
  uint16_t          &TCPPort()       { return TCPPort_; }

private:
  std::string IP_{"debug"}; // TODO: (`HUT`) : delete this
  uint16_t    TCPPort_ = 0;
};

}
}

#endif /* NETWORK_CLASSES_HPP */
