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

#include "oef-base/comms/EndpointWebSocket.hpp"

// #include "boost/asio/bind_executor.hpp"
#include "oef-base/monitoring/Gauge.hpp"
#include "oef-base/utils/Uri.hpp"

#include <utility>

// namespace websocket = boost::beast::websocket;

static Gauge wsep_count("mt-core.network.EndpointWebSocket");

template <typename TXType>
EndpointWebSocket<TXType>::EndpointWebSocket(asio::io_context & /*io_context*/,
                                             std::size_t sendBufferSize, std::size_t readBufferSize,
                                             ConfigMap configMap)
  : EndpointBase<TXType>(sendBufferSize, readBufferSize, configMap)
//  , web_socket_(io_context)
//  , strand_(web_socket_.get_executor())
{
  //  wsep_count++;
}

template <typename TXType>
EndpointWebSocket<TXType>::~EndpointWebSocket()
{
  //  wsep_count--;
}

template <typename TXType>
void EndpointWebSocket<TXType>::close()
{
  /*
    Lock lock(this->mutex);
    *state |= this->CLOSED_ENDPOINT;
    try
    {
      web_socket_.close(websocket::close_code::normal);
    }
    catch (std::exception &ec)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "WebSocket already closed!");
    }
    */
}

template <typename TXType>
void EndpointWebSocket<TXType>::go()
{
  /*
    FETCH_LOG_WARN(LOGGING_NAME, "Got new connection, probably WebSocket..");
    web_socket_.async_accept(asio::bind_executor(
        strand_,
        std::bind(&EndpointWebSocket::on_accept, shared_from_this(), std::placeholders::_1)));
  */
}

template <typename TXType>
void EndpointWebSocket<TXType>::async_write()
{
  /*
    auto data = sendBuffer.GetDataBuffers();

    int i = 0;
    for (auto &d : data)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Send buffer ", i, "=", d.size(),
                     " bytes on thr=", std::this_thread::get_id());
      ++i;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "run_sending: START");

    auto my_state = state;

    web_socket_.async_write(data, [this, my_state](std::error_code const &ec, const size_t &bytes) {
      this->complete_sending(my_state, ec, bytes);
    });
  */
}

template <typename TXType>
void EndpointWebSocket<TXType>::async_read(const std::size_t & /*bytes_needed*/)
{
  /*
    auto space    = readBuffer.GetSpaceBuffers();
    auto my_state = state;
    async_read_at_least(bytes_needed, 0, space, my_state);
  */
}

template <typename TXType>
void EndpointWebSocket<TXType>::async_read_at_least(
    const std::size_t & /*bytes_needed*/, std::size_t /*bytes_read*/,
    std::vector<RingBuffer::mutable_buffer> & /*space*/, std::shared_ptr<StateType> /*my_state*/)
{
  /*
    if (*state != this->RUNNING_ENDPOINT)
    {
      return;
    }
    if (bytes_read >= bytes_needed)
    {
      return;
    }

    web_socket_.async_read_some(space, [this, my_state, &bytes_read, bytes_needed, &space](
                                           std::error_code const &ec, const size_t &bytes) {
      bytes_read += bytes;
      if (bytes_read >= bytes_needed)
      {
        this->complete_reading(my_state, ec, bytes);
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Calling read_at_least again, bytes_needed: ", bytes_needed,
                       ", bytes_read: ", bytes_read);
        this->async_read_at_least(bytes_needed, bytes_read, space, my_state);
      }
    });
  */
}

template <typename TXType>
void EndpointWebSocket<TXType>::on_accept(std::error_code const & /*ec*/)
{
  //  FETCH_LOG_WARN(LOGGING_NAME, "WebSocket accepted");
  //  EndpointBase<TXType>::go();
  /*
    using namespace std::chrono_literals;
    if (ec)
    {
      FETCH_LOG_WARN("Error accepting web socket: ", ec.message());
      return;
    }
    //boost::beast::multi_buffer buffer;
    // Read a message into our buffer
    //web_socket_.read(buffer);

    auto space = readBuffer.GetSpaceBuffers();
    std::error_code errorCode;

    FETCH_LOG_INFO(LOGGING_NAME, "Reading...");
    auto bytes = web_socket_.read_some(space, errorCode);
    readBuffer.MarkSpaceUsed(bytes);
    FETCH_LOG_WARN(LOGGING_NAME, "Got data: ", bytes, " , ", readBuffer.GetDataAvailable());

    if (errorCode) {
      FETCH_LOG_WARN(LOGGING_NAME, "Error reading web socket: ", ec.message());
      return;
    }


    FETCH_LOG_INFO(LOGGING_NAME, "Getting data");
    auto data = readBuffer.GetDataBuffers();

    FETCH_LOG_INFO(LOGGING_NAME, "Write data: ", data.size(), ". data[0].size=", data[0].size());
    auto written = web_socket_.write(data, errorCode);

    if (errorCode) {
      FETCH_LOG_WARN(LOGGING_NAME, "Error writing web socket: ", ec.message());
      return;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Data written: ", written);
  */
}

template <typename TXType>
bool EndpointWebSocket<TXType>::is_eof(std::error_code const & /*ec*/) const
{
  return false;  // ec == boost::beast::websocket::error::closed;
}

template class EndpointWebSocket<std::shared_ptr<google::protobuf::Message>>;
template class EndpointWebSocket<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>;
