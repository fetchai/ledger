#pragma once
#include "core/mutex.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/byte_array.hpp"

#include<fstream>
#include<memory>

namespace fetch
{
namespace network
{
namespace details
{
  class Monitor
  {
  public:
    explicit Monitor(std::string filename)
    : stream_(filename, std::ios::out)
    {}

    void DumpIncomingMessage(std::string protocol, std::string type, byte_array::ConstByteArray const&msg)
    {
      FETCH_LOCK(mutex_);
      stream_ << "> " <<  protocol << " " << type << ": " << byte_array::ToBase64(msg) << std::endl << std::flush;
    }

    void DumpOutgoingMessage(std::string protocol, std::string type, byte_array::ConstByteArray const&msg)
    {
      FETCH_LOCK(mutex_);
      stream_ << "< " <<  protocol << " " << type << ": " << byte_array::ToBase64(msg) << std::endl << std::flush;
    }    

    void DumpIncomingMessage(std::string protocol, std::string type, char const *bytes, std::size_t n)
    {
      byte_array::ByteArray data;
      data.Resize(n);
      for (std::size_t i = 0; i < n; ++i)
      {
        data[i] = uint8_t(bytes[i]);
      }

      DumpIncomingMessage(protocol, type, data);
    }

    void DumpOutgoingMessage(std::string protocol, std::string type, char const *bytes, std::size_t n)
    {
      byte_array::ByteArray data;
      data.Resize(n);
      for (std::size_t i = 0; i < n; ++i)
      {
        data[i] = uint8_t(bytes[i]);
      }

      DumpOutgoingMessage(protocol, type, data);
    }

  private:
    std::fstream stream_;
    mutex::Mutex mutex_{__LINE__, __FILE__};
  };

}

  struct MonitoringClass
  {
    static std::shared_ptr< details::Monitor > monitor;     
  };

  inline void DumpNetworkActivityTo(std::string filename)
  {
    MonitoringClass::monitor = std::make_shared< details::Monitor >(filename);
  }
}
}


#define DUMP_INCOMING_MESSAGE(protocol, type, ...) \
  if(fetch::network::MonitoringClass::monitor) fetch::network::MonitoringClass::monitor->DumpIncomingMessage(protocol, type, __VA_ARGS__)

#define DUMP_OUTGOING_MESSAGE(protocol, type, ...) \
  if(fetch::network::MonitoringClass::monitor) fetch::network::MonitoringClass::monitor->DumpOutgoingMessage(protocol, type, __VA_ARGS__)
