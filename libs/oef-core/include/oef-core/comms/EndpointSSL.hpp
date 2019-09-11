#pragma once

#include "oef-base/comms/EndpointBase.hpp"
#include "oef-base/comms/ISocketOwner.hpp"
#include "oef-base/comms/RingBuffer.hpp"

#include "core/logging.hpp"
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

  virtual ~EndpointSSL();

  virtual Socket &socket() override
  {
    if (ssl_setup)
      return ssl_sock_p->next_layer();
    else
      return sock;
  }

  virtual void close() override;
  virtual void go() override;

  std::string agent_key()
  {
    if (*state == this->RUNNING_ENDPOINT)
      return agent_key_;
    else
      return "";
  }

  std::shared_ptr<EvpPublicKey> get_peer_ssl_key();

protected:
  virtual void async_read(const std::size_t &bytes_needed) override;
  virtual void async_write() override;
  virtual bool is_eof(const std::error_code &ec) const override;

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

bool verify_agent_certificate(bool, asio::ssl::verify_context &);
