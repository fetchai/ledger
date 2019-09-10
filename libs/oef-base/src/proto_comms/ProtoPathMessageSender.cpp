#include "ProtoPathMessageSender.hpp"

#include <google/protobuf/message.h>
#include "protos/src/protos/transport.pb.h"
#include "base/src/cpp/utils/Uri.hpp"
#include "base/src/cpp/monitoring/Counter.hpp"
#include "base/src/cpp/proto_comms/ProtoMessageEndpoint.hpp"


static Counter bytes_produced_counter("mt-core.comms.protopath.send.bytes_produced");
static Counter bytes_requested_counter("mt-core.comms.protopath.send.bytes_requested");
static Counter messages_handled_counter("mt-core.comms.protopath.send.messages_handled");

ProtoPathMessageSender::consumed_needed_pair ProtoPathMessageSender::checkForSpace(const mutable_buffers &data, IMessageWriter::TXQ& txq)
{
  FETCH_LOG_INFO(LOGGING_NAME, "search message tx...");
  CharArrayBuffer chars(data);
  std::ostream os(&chars);

  std::size_t consumed = 0;
  while(true)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Starting search message tx...");
    {
      auto ep = endpoint.lock();
      if (ep == nullptr)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "No endpoint pointer, break..");
        break;
      }

      if (!ep->IsTXQFull())
      {
        ep->wake();

        if (txq.empty())
        {
          FETCH_LOG_WARN(LOGGING_NAME, "TXQ empty, break..");
          break;
        }
      }

      Lock lock(mutex);

      TransportHeader leader;
      leader.set_uri(txq.front().first.path.substr(1));
      leader.set_id(txq.front().first.port);
      leader.mutable_status() -> set_success(true);
      uint32_t leader_head_size = sizeof(uint32_t);
      uint32_t payload_head_size = sizeof(uint32_t);

      uint32_t payload_size = txq.front().second -> ByteSize();
      uint32_t leader_size = leader.ByteSize();

      uint32_t mesg_size = leader_head_size + leader_size + payload_head_size + payload_size;
      if (chars.remainingSpace() < mesg_size)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "out of space on write buffer.");
        break;
      }

      chars.write(leader_size);
      chars.write(payload_size);
      leader.SerializeToOstream(&os);
      txq.front().second -> SerializeToOstream(&os);

      bytes_produced_counter += 8;
      bytes_produced_counter += leader_size;
      bytes_produced_counter += payload_size;

      txq.pop_front();
      messages_handled_counter++;
      //std::cout << "Ready for sending! bytes=" << mesg_size << std::endl;
      //chars.diagnostic();

      consumed += mesg_size;
    }
  }
  bytes_requested_counter += consumed;
  return consumed_needed_pair(consumed, 0);
}
