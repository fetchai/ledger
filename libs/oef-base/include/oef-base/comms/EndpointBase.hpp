#pragma once

#include "core/logging.hpp"
#include "network/fetch_asio.hpp"
#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/IMessageReader.hpp"
#include "oef-base/comms/IMessageWriter.hpp"
#include "oef-base/comms/ISocketOwner.hpp"
#include "oef-base/comms/RingBuffer.hpp"
#include "oef-base/threading/Notification.hpp"
#include "oef-base/threading/Waitable.hpp"

#include <iostream>
#include <list>

class Uri;

template <typename TXType>
class EndpointBase : public ISocketOwner, public Waitable
{
public:
  using Mutex     = std::mutex;
  using Lock      = std::lock_guard<Mutex>;
  using Socket    = asio::ip::tcp::socket;
  using ConfigMap = std::unordered_map<std::string, std::string>;

  using message_type = TXType;

  using ErrorNotification      = std::function<void(std::error_code const &ec)>;
  using EofNotification        = std::function<void()>;
  using StartNotification      = std::function<void()>;
  using ProtoErrorNotification = std::function<void(const std::string &message)>;

  using StateType  = std::atomic<int>;
  using StateTypeP = std::shared_ptr<StateType>;

  using EndpointState = enum {
    RUNNING_ENDPOINT = 1,
    CLOSED_ENDPOINT  = 2,
    EOF_ENDPOINT     = 4,
    ERRORED_ENDPOINT = 8
  };

  static constexpr std::size_t BUFFER_SIZE_LIMIT = 50;
  static constexpr char const *LOGGING_NAME      = "EndpointBase";

  EndpointBase(const EndpointBase &other) = delete;
  EndpointBase &operator=(const EndpointBase &other)  = delete;
  bool          operator==(const EndpointBase &other) = delete;
  bool          operator<(const EndpointBase &other)  = delete;

  EndpointBase(std::size_t sendBufferSize, std::size_t readBufferSize, ConfigMap configMap);
  virtual ~EndpointBase();

  virtual Socket &socket() = 0;

  virtual void go();

  std::shared_ptr<IMessageReader>         reader;
  std::shared_ptr<IMessageWriter<TXType>> writer;

  ErrorNotification      onError;
  EofNotification        onEof;
  StartNotification      onStart;
  ProtoErrorNotification onProtoError;

  virtual void run_sending();
  virtual void run_reading();
  virtual void close();
  virtual bool connect(const Uri &uri, Core &core);

  virtual const std::string &getRemoteId() const
  {
    return remote_id;
  }

  virtual Notification::NotificationBuilder send(TXType s);

  virtual bool IsTXQFull()
  {
    Lock lock(txq_mutex);
    return txq.size() >= BUFFER_SIZE_LIMIT;
  }

  virtual bool connected()
  {
    if (*state > RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "STATE: ", state);
    }
    return *state == RUNNING_ENDPOINT;
  }

  std::size_t getIdent(void) const
  {
    return ident;
  }

protected:
  virtual void async_read(const std::size_t &bytes_needed) = 0;
  virtual void async_write()                               = 0;

  virtual bool is_eof(std::error_code const &ec) const = 0;

protected:
  RingBuffer sendBuffer;
  RingBuffer readBuffer;

  ConfigMap configMap_;

  Mutex       mutex;
  Mutex       txq_mutex;
  std::size_t read_needed = 0;
  std::size_t ident;

  std::string remote_id;

  std::atomic<bool> asio_sending;
  std::atomic<bool> asio_reading;

  std::shared_ptr<StateType> state;

  virtual void error(std::error_code const &ec);
  virtual void proto_error(const std::string &msg);
  virtual void eof();

  virtual void complete_sending(StateTypeP state, std::error_code const &ec, const size_t &bytes);
  virtual void create_messages();
  virtual void complete_reading(StateTypeP state, std::error_code const &ec, const size_t &bytes);

private:
  std::vector<Notification::Notification> waiting;
  std::list<TXType>                       txq;
};
