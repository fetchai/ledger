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
#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/monitoring/Gauge.hpp"
#include "oef-base/utils/Uri.hpp"
#include <cstdlib>

static Gauge ep_count("mt-core.network.Endpoint");

template <typename TXType>
Endpoint<TXType>::Endpoint(Core &core, std::size_t sendBufferSize, std::size_t readBufferSize,
                           ConfigMap configMap)
  : EndpointBase<TXType>(sendBufferSize, readBufferSize, configMap)
  , sock(static_cast<asio::io_context &>(core))
{
  ep_count++;
}

template <typename TXType>
Endpoint<TXType>::~Endpoint()
{
  ep_count--;
}

template <typename TXType>
void Endpoint<TXType>::async_write()
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
  asio::async_write(sock, data, [this, my_state](std::error_code const &ec, const size_t &bytes) {
    this->complete_sending(my_state, ec, bytes);
  });
}

template <typename TXType>
void Endpoint<TXType>::async_read(const std::size_t &bytes_needed)
{
  auto space    = readBuffer.GetSpaceBuffers();
  auto my_state = state;

  asio::async_read(sock, space, asio::transfer_at_least(bytes_needed),
                   [this, my_state](std::error_code const &ec, const size_t &bytes) {
                     this->complete_reading(my_state, ec, bytes);
                   });
}

template <typename TXType>
bool Endpoint<TXType>::is_eof(std::error_code const &ec) const
{
  return ec == asio::error::eof;
}

template class Endpoint<std::shared_ptr<google::protobuf::Message>>;
template class Endpoint<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>;
