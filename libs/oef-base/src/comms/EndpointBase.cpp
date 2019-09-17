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

#include "oef-base/comms/EndpointBase.hpp"
#include "logging/logging.hpp"

// TODO: Replace beast #include "boost/beast/websocket/error.hpp"
#include "oef-base/monitoring/Gauge.hpp"
#include "oef-base/utils/Uri.hpp"
#include <cstdlib>

static Gauge                    count("mt-core.network.EndpointBase");
static std::atomic<std::size_t> endpoint_ident(1000);

template <typename TXType>
bool EndpointBase<TXType>::connect(const Uri &uri, Core &core)
{
  asio::ip::tcp::resolver        resolver(core);
  asio::ip::tcp::resolver::query query(uri.host, std::to_string(uri.port));
  auto                           results = resolver.resolve(query);
  std::error_code                ec;

  for (auto &endpoint : results)
  {
    socket().connect(endpoint);
    if (ec)
    {
      // An error occurred.
      std::cout << ec.value() << std::endl;
    }
    else
    {
      *state_ = RUNNING_ENDPOINT;
      return true;
    }
  }
  ident_ = endpoint_ident++;
  return false;
}

template <typename TXType>
EndpointBase<TXType>::EndpointBase(std::size_t sendBufferSize, std::size_t readBufferSize,
                                   ConfigMap configMap)
  : sendBuffer_(sendBufferSize)
  , readBuffer_(readBufferSize)
  , configMap_(std::move(configMap))
  , asio_sending_(false)
  , asio_reading_(false)
{
  state_ = std::make_shared<StateType>(0);
  count++;
  ident_ = endpoint_ident++;
}

template <typename TXType>
EndpointBase<TXType>::~EndpointBase()
{
  count--;
}

template <typename TXType>
void EndpointBase<TXType>::run_sending()
{
  {
    Lock lock(mutex_);
    if (asio_sending_ || *state_ != RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "early exit 1 sending=", asio_sending_, " state=", *state_);
      return;
    }
    if (sendBuffer_.GetDataAvailable() == 0)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "create messages");
      create_messages();
    }
    if (sendBuffer_.GetDataAvailable() == 0)
    {
      return;
    }
    asio_sending_ = true;
  }
  FETCH_LOG_DEBUG(LOGGING_NAME, "ok data available");

  async_write();
}

template <typename TXType>
void EndpointBase<TXType>::run_reading()
{
  std::size_t read_needed_local = 0;

  FETCH_LOG_DEBUG(LOGGING_NAME, reader_.get(), "::run_reading");
  {
    Lock lock(mutex_);
    if (asio_reading_ || *state_ != RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, reader_.get(), ":early exit 1 reading=", asio_sending_,
                     " state=", *state_);
      return;
    }
    if (read_needed_ == 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME, reader_.get(), ":early exit 1 read_needed=", read_needed_,
                     " state=", *state_);
      return;
    }
    read_needed_local = read_needed_;
    if (read_needed_local > readBuffer_.GetFreeSpace())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "********** READ BUFFER FULL!");
      read_needed_local = readBuffer_.GetFreeSpace();
    }
    read_needed_  = 0;
    asio_reading_ = true;
  }

  if (read_needed_local == 0)
  {
    return;
  }

  async_read(read_needed_local);
}

template <typename TXType>
void EndpointBase<TXType>::close()
{
  Lock lock(mutex_);
  *state_ |= CLOSED_ENDPOINT;
  socket().close();
}

template <typename TXType>
void EndpointBase<TXType>::eof()
{
  if (*state_ & EOF_ENDPOINT)
  {
    return;
  }

  EofNotification local_handler_copy;
  {
    Lock lock(mutex_);
    *state_ |= EOF_ENDPOINT;
    *state_ |= CLOSED_ENDPOINT;
    socket().close();

    local_handler_copy = onEof;
    onEof              = 0;
    onError            = 0;
    onProtoError       = 0;
  }

  if (local_handler_copy)
  {
    try
    {
      local_handler_copy();
    }
    catch (...)
    {
    }  // Ignore exceptions.
  }
}

template <typename TXType>
void EndpointBase<TXType>::error(std::error_code const &ec)
{
  if (*state_ & ERRORED_ENDPOINT)
  {
    return;
  }

  ErrorNotification local_handler_copy;

  {
    Lock lock(mutex_);
    *state_ |= ERRORED_ENDPOINT;
    *state_ |= CLOSED_ENDPOINT;
    socket().close();

    local_handler_copy = onError;
    onEof              = 0;
    onError            = 0;
    onProtoError       = 0;
  }

  if (local_handler_copy)
  {
    try
    {
      local_handler_copy(ec);
    }
    catch (...)
    {
    }  // Ignore exceptions.
  }
}

template <typename TXType>
void EndpointBase<TXType>::proto_error(const std::string &msg)
{
  // std::cout << "proto error: " << msg << std::endl;

  if (*state_ & ERRORED_ENDPOINT)
  {
    return;
  }

  ProtoErrorNotification local_handler_copy;

  {
    Lock lock(mutex);
    *state_ |= ERRORED_ENDPOINT;
    *state_ |= CLOSED_ENDPOINT;
    socket().close();

    local_handler_copy = onProtoError;
    onEof              = 0;
    onError            = 0;
    onProtoError       = 0;
  }

  if (local_handler_copy)
  {
    try
    {
      local_handler_copy(msg);
    }
    catch (...)
    {
    }  // Ignore exceptions.
  }
}

template <typename TXType>
void EndpointBase<TXType>::go()
{
  remote_id_ = socket().remote_endpoint().address().to_string();
  FETCH_LOG_INFO(LOGGING_NAME, "remote_id detected as: ", remote_id_);
  asio::socket_base::linger option(false, 0);
  socket().set_option(option);

  if (onStart)
  {
    auto myStart = onStart;
    onStart      = 0;
    try
    {
      myStart();
    }
    catch (...)
    {
      Lock lock(mutex_);
      *state_ |= ERRORED_ENDPOINT;
      *state_ |= CLOSED_ENDPOINT;
      socket().close();
    }
  }

  *state_ |= RUNNING_ENDPOINT;

  {
    Lock lock(mutex_);
    read_needed_++;
  }

  run_reading();
}

template <typename TXType>
void EndpointBase<TXType>::complete_sending(StateTypeP state, std::error_code const &ec,
                                            const std::size_t &bytes)
{
  try
  {
    if (*state > RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "complete_sending on already dead socket", ec);
      return;
    }

    if (is_eof(ec) || ec == asio::error::operation_aborted)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "complete_sending EOF:  ", ec);
      *state |= CLOSED_ENDPOINT;
      eof();
      return;  // We are done with this thing!
    }

    if (ec)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "complete_sending ERR:  ", ec);
      error(ec);
      return;  // We are done with this thing!
    }

    // std::cout << "complete_sending OK:  " << bytes << std::endl;
    sendBuffer_.MarkDataUsed(bytes);
    create_messages();
    {
      Lock lock(mutex_);
      asio_sending_ = false;
    }
    // std::cout << "complete_sending: kick" << std::endl;
    run_sending();
  }

  catch (std::exception &ex)
  {
    close();
    return;
  }
}

template <typename TXType>
void EndpointBase<TXType>::create_messages()
{
  auto consumed_needed = writer_->CheckForSpace(sendBuffer_.GetSpaceBuffers(), txq_);
  auto consumed        = consumed_needed.first;
  sendBuffer_.MarkSpaceUsed(consumed);
}

template <typename TXType>
void EndpointBase<TXType>::complete_reading(StateTypeP state, std::error_code const &ec,
                                            const std::size_t &bytes)
{
  try
  {
    if (*state > RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "complete_reading on already dead socket", ec);
      return;
    }

    // std::cout << reader.get() << ":  complete_reading:  " << ec << ", "<< bytes << std::endl;

    if (is_eof(ec) || ec == asio::error::operation_aborted)
    {
      // std::cout << "complete_reading: eof" << std::endl;
      *state |= CLOSED_ENDPOINT;
      eof();
      return;  // We are done with this thing!
    }

    if (ec)
    {
      // std::cout << "complete_reading: error: " << ec.message() << " "<< ec.value() << std::endl;
      *state |= ERRORED_ENDPOINT;
      error(ec);
      close();
      return;  // We are done with this thing!
    }

    // std::cout << reader.get() << ":complete_reading: 1 " << std::endl;
    readBuffer_.MarkSpaceUsed(bytes);

    // std::cout << reader.get() << ":complete_reading: 2" << std::endl;
    IMessageReader::consumed_needed_pair consumed_needed;

    // std::cout << reader.get() << ":complete_reading: 3" << std::endl;
    try
    {
      // std::cout << reader.get() << ":complete_reading: 4" << std::endl;
      // std::cout << reader.get() << ": @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ " <<
      // reader.get() << std::endl;
      consumed_needed = reader_->CheckForMessage(readBuffer_.GetDataBuffers());
      // std::cout << "     DONE." << std::endl;
    }
    catch (std::exception &ex)
    {
      // std::cout << "complete_reading: 5" << std::endl;
      proto_error(ex.what());
      return;
    }
    catch (...)
    {
      // std::cout << "complete_reading: 6" << std::endl;
      proto_error("unknown protocol error");
      return;
    }

    // std::cout << "complete_reading: 7" << std::endl;
    auto consumed = consumed_needed.first;
    auto needed   = consumed_needed.second;

    readBuffer_.MarkDataUsed(consumed);

    {
      Lock lock(mutex_);
      read_needed_ = needed;
    }

    {
      Lock lock(mutex_);
      asio_reading_ = false;
    }
    run_reading();
  }
  catch (std::exception &ex)
  {
    close();
    return;
  }
}

template <typename TXType>
Notification::NotificationBuilder EndpointBase<TXType>::send(TXType s)
{
  Lock lock(txq_mutex_);
  if (txq_.size() < BUFFER_SIZE_LIMIT)
  {
    txq_.push_back(s);
    return Notification::NotificationBuilder();
  }
  else
  {
    return MakeNotification();
  }
}

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

template class EndpointBase<std::shared_ptr<google::protobuf::Message>>;
template class EndpointBase<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>;
