#include "Endpoint.hpp"
#include "fetch_teams/ledger/logger.hpp"

#include "base/src/cpp/utils/Uri.hpp"
#include <cstdlib>
#include "base/src/cpp/monitoring/Gauge.hpp"

static Gauge ep_count("mt-core.network.Endpoint");

template <typename TXType>
Endpoint<TXType>::Endpoint(
      Core &core
      ,std::size_t sendBufferSize
      ,std::size_t readBufferSize
      ,ConfigMap configMap
  )
    : EndpointBase<TXType>(sendBufferSize, readBufferSize, configMap)
    , sock(core)
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
  auto data = sendBuffer.getDataBuffers();

  int i = 0;
  for(auto &d : data)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Send buffer ", i, "=", d.size(), " bytes on thr=", std::this_thread::get_id());
    ++i;
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "run_sending: START");

  auto my_state = state;
  boost::asio::async_write(
                           sock,
                           data,
                           [this, my_state](const  boost::system::error_code& ec, const size_t &bytes){
                             this -> complete_sending(my_state, ec, bytes);
                           }
                           );
}

template <typename TXType>
void Endpoint<TXType>::async_read(const std::size_t& bytes_needed)
{
  auto space = readBuffer.getSpaceBuffers();
  auto my_state = state;

  boost::asio::async_read(
                          sock,
                          space,
                          boost::asio::transfer_at_least(bytes_needed),
                          [this, my_state](const boost::system::error_code& ec, const size_t &bytes){
                            this -> complete_reading(my_state, ec, bytes);
                          }
                          );
}

template <typename TXType>
bool Endpoint<TXType>::is_eof(const boost::system::error_code& ec) const
{
  return ec == boost::asio::error::eof;
}

template class Endpoint<std::shared_ptr<google::protobuf::Message>>;
template class Endpoint<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>>;