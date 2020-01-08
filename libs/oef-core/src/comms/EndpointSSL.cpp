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

#include "logging/logging.hpp"
#include "oef-core/comms/EndpointSSL.hpp"

#include "oef-base/monitoring/Gauge.hpp"
#include "oef-base/utils/Uri.hpp"

#include <cerrno>  // for fopen
#include <cstdio>  // for fopen
#include <functional>
#include <memory>
#include <openssl/ssl.h>
#include <system_error>

using std::placeholders::_1;
using std::placeholders::_2;

static Gauge ep_count("mt-core.network.EndpointSSL");

template <typename TXType>
EndpointSSL<TXType>::EndpointSSL(Core &core, std::size_t sendBufferSize, std::size_t readBufferSize,
                                 ConfigMap configMap)
  : EndpointBase<TXType>(sendBufferSize, readBufferSize, configMap)
  , sock(static_cast<asio::io_context &>(core))
{
  ep_count++;
  ssl_setup = false;
  try
  {
    ssl_ctx_p  = make_ssl_ctx();
    ssl_sock_p = new SocketSSL(std::move(sock), *ssl_ctx_p);
    ssl_sock_p->set_verify_mode(asio::ssl::verify_peer);
    ssl_sock_p->set_verify_callback(std::bind(&verify_agent_certificate, _1, _2));
  }
  catch (std::exception const &ec)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "SSL context initialization: ", ec.what());
    return;
  }
  ssl_setup = true;
}

template <typename TXType>
EndpointSSL<TXType>::~EndpointSSL()
{
  FETCH_LOG_INFO(LOGGING_NAME, "~EndpointSSL<>");
  ep_count--;
  delete ssl_ctx_p;
  delete ssl_sock_p;
}

template <typename TXType>
void EndpointSSL<TXType>::close()
{
  if (!ssl_setup)
  {
    return;
  }

  Lock lock(this->mutex);
  *state |= this->CLOSED_ENDPOINT;
  try
  {
    ssl_sock_p->shutdown();
    ssl_sock_p->lowest_layer().shutdown(asio::socket_base::shutdown_type::shutdown_both);

    ssl_sock_p->lowest_layer().close();
  }
  catch (std::exception const &ec)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "SSL Socket when closing: ", ec.what());
  }
}

template <typename TXType>
void EndpointSSL<TXType>::go()
{
  FETCH_LOG_WARN(LOGGING_NAME, "Got new connection, attempting ssl handshake ...");
  ssl_sock_p->async_handshake(asio::ssl::stream_base::server, [this](const std::error_code &error) {
    if (!error)
    {
      try
      {
        X509CertP    cert{ssl_sock_p->native_handle()};
        EvpPublicKey pk{cert};

        agent_ssl_key_ = RSA_Modulus(pk);
        FETCH_LOG_INFO(LOGGING_NAME, "Got Agent PubKey: ", agent_ssl_key_);
      }
      catch (std::exception const &e)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Couldn't get agent public key from ssl socket : ", e.what());
        return;
      }
      FETCH_LOG_WARN(LOGGING_NAME, "SSL handshake successfull");
      EndpointBase<TXType>::go();
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "SSL handshake failed: ", error.message());
      close();
    }
  });
}

template <typename TXType>
std::shared_ptr<EvpPublicKey> EndpointSSL<TXType>::get_peer_ssl_key()
{
  X509CertP cert{ssl_sock_p->native_handle()};
  return std::make_shared<EvpPublicKey>(cert);
}

bool verify_agent_certificate(bool preverified, asio::ssl::verify_context &ctx)
{
  // Accept any
  char  subject_name[256];
  X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
  X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
  FETCH_LOG_INFO("EndpointSSL", "Certificate: ", subject_name, ", preverified: ", preverified);

  return true;
}

template <typename TXType>
void EndpointSSL<TXType>::async_write()
{
  auto data = sendBuffer.GetDataBuffers();

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

  int i = 0;
  for (auto &d : data)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Send buffer ", i, "=", d.size(),
                    " bytes on thr=", std::this_thread::get_id());
    ++i;
  }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  FETCH_LOG_DEBUG(LOGGING_NAME, "run_sending: START");

  auto my_state = state;
  asio::async_write(*ssl_sock_p, data,
                    [this, my_state](const std::error_code &ec, const size_t &bytes) {
                      this->complete_sending(my_state, ec, bytes);
                    });
}

template <typename TXType>
void EndpointSSL<TXType>::async_read(const std::size_t &bytes_needed)
{
  auto space    = readBuffer.GetSpaceBuffers();
  auto my_state = state;

  FETCH_LOG_DEBUG(LOGGING_NAME, "run_reading: START, bytes_needed: ", bytes_needed);

  // auto self = shared_from_this();
  asio::async_read(*ssl_sock_p, space, asio::transfer_at_least(bytes_needed),
                   [this, my_state](const std::error_code &ec, const size_t &bytes) {
                     this->complete_reading(my_state, ec, bytes);
                   });
}

template <typename TXType>
bool EndpointSSL<TXType>::is_eof(const std::error_code &ec) const
{
  return ec == asio::error::eof;
}

template <typename TXType>
typename EndpointSSL<TXType>::ContextSSL *EndpointSSL<TXType>::make_ssl_ctx()
{
  auto ssl_ctx = new ContextSSL(asio::ssl::context::tlsv12);
  ssl_ctx->set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2);
  //| asio::ssl::context::single_dh_use);
  auto sk_f = configMap_.find("core_cert_pk_file");
  if (sk_f == configMap_.end())
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "SSL setup failed, because missing core_cert_pk_file from configuration!");
    return nullptr;
  }
  ssl_ctx->use_certificate_chain_file(sk_f->second);
  ssl_ctx->use_private_key_file(sk_f->second, asio::ssl::context::pem);
  auto dh_file = configMap_.find("tmp_dh_file");
  if (dh_file == configMap_.end())
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "SSL setup failed, because missing tmp_dh_file from configuration!");
    return nullptr;
  }
  ssl_ctx->use_tmp_dh_file(dh_file->second);

  // impose cipher
  SSL_CTX_set_cipher_list(ssl_ctx->native_handle(), "DHE-RSA-AES256-SHA256");

  return ssl_ctx;
}

template class EndpointSSL<std::shared_ptr<google::protobuf::Message>>;
template class EndpointSSL<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>;
