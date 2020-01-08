#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "oef-base/comms/EndpointBase.hpp"
#include "oef-base/comms/ISocketOwner.hpp"
#include "oef-base/comms/RingBuffer.hpp"

#include "logging/logging.hpp"
#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/IMessageReader.hpp"
#include "oef-base/comms/IMessageWriter.hpp"
#include "oef-core/comms/public_key_utils.hpp"

#include "network/fetch_asio.hpp"

#include <openssl/evp.h>

template <typename TXType>
class EndpointSSL : public EndpointBase<TXType>,
                    public std::enable_shared_from_this<EndpointSSL<TXType>>
{
public:
  using EndpointBase<TXType>::state;
  using EndpointBase<TXType>::readBuffer;
  using EndpointBase<TXType>::sendBuffer;
  using EndpointBase<TXType>::configMap_;
  using typename EndpointBase<TXType>::message_type;
  using std::enable_shared_from_this<EndpointSSL<TXType>>::shared_from_this;

  using ConfigMap  = typename EndpointBase<TXType>::ConfigMap;
  using Socket     = typename EndpointBase<TXType>::Socket;
  using SocketSSL  = asio::ssl::stream<Socket>;
  using ContextSSL = asio::ssl::context;
  using Lock       = typename EndpointBase<TXType>::Lock;

  static constexpr char const *LOGGING_NAME = "EndpointSSL";

  EndpointSSL(const EndpointSSL &other) = delete;
  EndpointSSL &operator=(const EndpointSSL &other)  = delete;
  bool         operator==(const EndpointSSL &other) = delete;
  bool         operator<(const EndpointSSL &other)  = delete;

  EndpointSSL(Core &core, std::size_t sendBufferSize, std::size_t readBufferSize,
              ConfigMap configMap);

  ~EndpointSSL() override;

  Socket &socket() override
  {
    if (ssl_setup)
    {
      return ssl_sock_p->next_layer();
    }
    else
    {
      return sock;
    }
  }

  void close() override;
  void go() override;

  std::string agent_key()
  {
    if (*state == this->RUNNING_ENDPOINT)
    {
      return agent_key_;
    }
    else
    {
      return "";
    }
  }

  std::shared_ptr<EvpPublicKey> get_peer_ssl_key();

protected:
  void async_read(const std::size_t &bytes_needed) override;
  void async_write() override;
  bool is_eof(const std::error_code &ec) const override;

private:
  SocketSSL * ssl_sock_p;
  ContextSSL *ssl_ctx_p;
  std::string key_;
  bool        ssl_setup;

  Socket sock;

  ContextSSL *make_ssl_ctx();

  std::string agent_key_;
  std::string agent_ssl_key_;
};

bool verify_agent_certificate(bool /*preverified*/, asio::ssl::verify_context & /*ctx*/);
