#pragma once

#include "core/logging.hpp"
#include "network/fetch_asio.hpp"
#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/EndpointBase.hpp"
#include "oef-base/comms/IMessageReader.hpp"
#include "oef-base/comms/IMessageWriter.hpp"
#include "oef-base/comms/ISocketOwner.hpp"
#include "oef-base/comms/RingBuffer.hpp"

#include <iostream>

template <typename TXType>
class Endpoint : public EndpointBase<TXType>, public std::enable_shared_from_this<Endpoint<TXType>>
{
public:
  using EndpointBase<TXType>::state;
  using EndpointBase<TXType>::readBuffer;
  using EndpointBase<TXType>::sendBuffer;
  using typename EndpointBase<TXType>::message_type;

  using ConfigMap = typename EndpointBase<TXType>::ConfigMap;
  using Socket    = typename EndpointBase<TXType>::Socket;

  static constexpr char const *LOGGING_NAME = "Endpoint";

  Endpoint(const Endpoint &other) = delete;
  Endpoint &operator=(const Endpoint &other)  = delete;
  bool      operator==(const Endpoint &other) = delete;
  bool      operator<(const Endpoint &other)  = delete;

  Endpoint(Core &core, std::size_t sendBufferSize, std::size_t readBufferSize, ConfigMap configMap);

  virtual ~Endpoint();

  virtual Socket &socket() override
  {
    return sock;
  }

protected:
  virtual void async_read(const std::size_t &bytes_needed) override;
  virtual void async_write() override;
  virtual bool is_eof(std::error_code const &ec) const override;

protected:
  Socket sock;
};
