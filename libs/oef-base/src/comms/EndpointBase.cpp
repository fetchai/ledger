#include "oef-base/comms/EndpointBase.hpp"
#include "boost/beast/websocket/error.hpp"
#include "logging/logging.hpp"
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

  FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " outbound connection to ", uri.host, ":", uri.port);

  for (auto &endpoint : results)
  {
    socket().connect(endpoint);
    if (ec)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " outbound connection to ", uri.host, ":",
                     uri.port, " FAILED ", ec.value());
    }
    else
    {
      *state = RUNNING_ENDPOINT;
      return true;
    }
  }
  return false;
}

template <typename TXType>
EndpointBase<TXType>::EndpointBase(std::size_t sendBufferSize, std::size_t readBufferSize,
                                   ConfigMap configMap)
  : sendBuffer(sendBufferSize)
  , readBuffer(readBufferSize)
  , configMap_(std::move(configMap))
  , asio_sending(false)
  , asio_reading(false)
{
  state = std::make_shared<StateType>(0);
  count++;
  ident = endpoint_ident++;
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
    Lock lock(mutex);
    if (asio_sending || *state != RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " early exit 1 sending=", asio_sending,
                     " state=", *state);
      return;
    }
    if (sendBuffer.getDataAvailable() == 0)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "id=", ident, " create messages");
      create_messages();
    }
    if (sendBuffer.getDataAvailable() == 0)
    {
      return;
    }
    asio_sending = true;
  }
  FETCH_LOG_DEBUG(LOGGING_NAME, "id=", ident, " ok data available");

  async_write();
}

template <typename TXType>
void EndpointBase<TXType>::run_reading()
{
  std::size_t read_needed_local = 0;

  FETCH_LOG_DEBUG(LOGGING_NAME, reader.get(), "::run_reading");
  {
    Lock lock(mutex);
    if (asio_reading || *state != RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, reader.get(), ":early exit 1 reading=", asio_sending,
                     " state=", *state);
      return;
    }
    if (read_needed == 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME, reader.get(), ":early exit 1 read_needed=", read_needed,
                     " state=", *state);
      return;
    }
    read_needed_local = read_needed;
    if (read_needed_local > readBuffer.getFreeSpace())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "id=", ident, " ********** READ BUFFER FULL!");
      read_needed_local = readBuffer.getFreeSpace();
    }
    read_needed  = 0;
    asio_reading = true;
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
  if (*state & CLOSED_ENDPOINT)
  {
    return;
  }
  EofNotification local_handler_copy;
  {
    Lock lock(mutex);
    *state |= EOF_ENDPOINT;
    *state |= CLOSED_ENDPOINT;
    FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED CLOSED AT close() ");
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
      Lock lock(mutex);
      local_handler_copy();
    }
    catch (...)
    {
    }  // Ignore exceptions.
  }
}
template <typename TXType>
void EndpointBase<TXType>::eof()
{
  if (*state & EOF_ENDPOINT)
  {
    return;
  }

  EofNotification local_handler_copy;
  {
    Lock lock(mutex);
    *state |= EOF_ENDPOINT;
    *state |= CLOSED_ENDPOINT;
    socket().close();
    FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED EOF AT eof() ");

    local_handler_copy = onEof;
    onEof              = 0;
    onError            = 0;
    onProtoError       = 0;
  }

  if (local_handler_copy)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " doing onEof AT oef() ");
    try
    {
      Lock lock(mutex);
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
  if (*state & ERRORED_ENDPOINT)
  {
    return;
  }

  ErrorNotification local_handler_copy;

  {
    Lock lock(mutex);
    *state |= ERRORED_ENDPOINT;
    FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED ERROR AT error(ec) ", ec.message());
    *state |= CLOSED_ENDPOINT;
    socket().close();

    local_handler_copy = onError;
    onEof              = 0;
    onError            = 0;
    onProtoError       = 0;
  }

  if (local_handler_copy)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " doing onError AT error(ec) ", ec.message());
    try
    {
      Lock lock(mutex);
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
  if (*state & ERRORED_ENDPOINT)
  {
    return;
  }

  ProtoErrorNotification local_handler_copy;

  {
    Lock lock(mutex);
    *state |= ERRORED_ENDPOINT;
    *state |= CLOSED_ENDPOINT;
    FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED ERROR AT proto_error($) ", msg);
    socket().close();

    local_handler_copy = onProtoError;
    onEof              = 0;
    onError            = 0;
    onProtoError       = 0;
  }

  if (local_handler_copy)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " doing onProtoError AT proto_error($) ", msg);
    try
    {
      Lock lock(mutex);
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
  remote_id = socket().remote_endpoint().address().to_string();
  FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " remote_id detected as: ", remote_id);
  asio::socket_base::linger option(false, 0);
  socket().set_option(option);

  if (onStart)
  {
    auto myStart = onStart;
    onStart      = 0;
    try
    {
      myStart();
      *state |= RUNNING_ENDPOINT;
    }
    catch (...)
    {
      Lock lock(mutex);
      *state |= ERRORED_ENDPOINT;
      FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED ERROR AT go()");
      *state |= CLOSED_ENDPOINT;
      socket().close();
    }
  }

  {
    Lock lock(mutex);
    read_needed++;
  }

  run_reading();
}

template <typename TXType>
void EndpointBase<TXType>::complete_sending(StateTypeP state, std::error_code const &ec,
                                            const size_t &bytes)
{
  try
  {
    {
      if (*state > RUNNING_ENDPOINT)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " complete_sending on already dead socket", ec);
        return;
      }
    }

    if (is_eof(ec) || ec == asio::error::operation_aborted)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED EOF AT complete_sending ", ec.message());
      *state |= CLOSED_ENDPOINT;
      eof();
      return;  // We are done with this thing!
    }

    if (ec)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " complete_sending ERR:  ", ec);
      error(ec);
      return;  // We are done with this thing!
    }

    sendBuffer.markDataUsed(bytes);
    create_messages();
    {
      Lock lock(mutex);
      asio_sending = false;
    }
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
  auto consumed_needed = writer->checkForSpace(sendBuffer.getSpaceBuffers(), txq);
  auto consumed        = consumed_needed.first;
  sendBuffer.markSpaceUsed(consumed);
}

template <typename TXType>
void EndpointBase<TXType>::complete_reading(StateTypeP state, std::error_code const &ec,
                                            const size_t &bytes)
{
  try
  {
    {
      Lock lock(mutex);
      if (*state > RUNNING_ENDPOINT)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " complete_reading on already dead socket", ec);
        return;
      }
    }

    if (is_eof(ec) || ec == asio::error::operation_aborted)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED EOF AT complete_reading ", ec.message());
      eof();
      return;  // We are done with this thing!
    }

    if (ec)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " MARKED ERROR AT complete_reading() ",
                     ec.message());
      error(ec);
      close();
      return;  // We are done with this thing!
    }

    readBuffer.markSpaceUsed(bytes);

    IMessageReader::consumed_needed_pair consumed_needed;

    try
    {
      if (*state > RUNNING_ENDPOINT)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "id=", ident, " complete_reading on already dead socket", ec);
        return;
      }
      Lock lock(mutex);
      consumed_needed = reader->checkForMessage(readBuffer.getDataBuffers());
    }
    catch (std::exception &ex)
    {
      proto_error(ex.what());
      return;
    }
    catch (...)
    {
      proto_error("unknown protocol error");
      return;
    }

    auto consumed = consumed_needed.first;
    auto needed   = consumed_needed.second;

    readBuffer.markDataUsed(consumed);

    {
      Lock lock(mutex);
      read_needed = needed;
    }

    {
      Lock lock(mutex);
      asio_reading = false;
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
  Lock lock(txq_mutex);
  if (txq.size() < BUFFER_SIZE_LIMIT)
  {
    txq.push_back(s);
    return Notification::NotificationBuilder();
  }
  else
  {
    return makeNotification();
  }
}

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

template class EndpointBase<std::shared_ptr<google::protobuf::Message>>;
template class EndpointBase<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>;
