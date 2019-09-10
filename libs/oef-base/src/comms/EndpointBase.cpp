#include "oef-base/comms/EndpointBase.hpp"
#include "core/logging.hpp"

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
      *state = RUNNING_ENDPOINT;
      return true;
    }
  }
  ident = endpoint_ident++;
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
      FETCH_LOG_INFO(LOGGING_NAME, "early exit 1 sending=", asio_sending, " state=", *state);
      return;
    }
    if (sendBuffer.getDataAvailable() == 0)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "create messages");
      create_messages();
    }
    if (sendBuffer.getDataAvailable() == 0)
    {
      return;
    }
    asio_sending = true;
  }
  FETCH_LOG_DEBUG(LOGGING_NAME, "ok data available");

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
      FETCH_LOG_ERROR(LOGGING_NAME, "********** READ BUFFER FULL!");
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
  Lock lock(mutex);
  *state |= CLOSED_ENDPOINT;
  socket().close();
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
  if (*state & ERRORED_ENDPOINT)
  {
    return;
  }

  ErrorNotification local_handler_copy;

  {
    Lock lock(mutex);
    *state |= ERRORED_ENDPOINT;
    *state |= CLOSED_ENDPOINT;
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

  if (*state & ERRORED_ENDPOINT)
  {
    return;
  }

  ProtoErrorNotification local_handler_copy;

  {
    Lock lock(mutex);
    *state |= ERRORED_ENDPOINT;
    *state |= CLOSED_ENDPOINT;
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
  remote_id = socket().remote_endpoint().address().to_string();
  FETCH_LOG_INFO(LOGGING_NAME, "remote_id detected as: ", remote_id);
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
      Lock lock(mutex);
      *state |= ERRORED_ENDPOINT;
      *state |= CLOSED_ENDPOINT;
      socket().close();
    }
  }

  *state |= RUNNING_ENDPOINT;

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
    sendBuffer.markDataUsed(bytes);
    create_messages();
    {
      Lock lock(mutex);
      asio_sending = false;
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
    readBuffer.markSpaceUsed(bytes);

    // std::cout << reader.get() << ":complete_reading: 2" << std::endl;
    IMessageReader::consumed_needed_pair consumed_needed;

    // std::cout << reader.get() << ":complete_reading: 3" << std::endl;
    try
    {
      // std::cout << reader.get() << ":complete_reading: 4" << std::endl;
      // std::cout << reader.get() << ": @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ " <<
      // reader.get() << std::endl;
      consumed_needed = reader->checkForMessage(readBuffer.getDataBuffers());
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
