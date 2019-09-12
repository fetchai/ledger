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

#include "logging/logging.hpp"
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

  Endpoint(Endpoint const &other) = delete;
  Endpoint &operator=(Endpoint const &other)  = delete;
  bool      operator==(Endpoint const &other) = delete;
  bool      operator<(Endpoint const &other)  = delete;

  Endpoint(Core &core, std::size_t sendBufferSize, std::size_t readBufferSize, ConfigMap configMap);

  virtual ~Endpoint();

  virtual Socket &socket() override
  {
    return sock;
  }

protected:
  virtual void async_read(std::size_t const &bytes_needed) override;
  virtual void async_write() override;
  virtual bool is_eof(std::error_code const &ec) const override;

protected:
  Socket sock;
};
