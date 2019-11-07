#include "dmlf/colearn/muddle_outbound_update_task.hpp"
#include "dmlf/colearn/colearn_protocol.hpp"
#include "core/byte_array/decoders.hpp"
#include "muddle/rpc/client.hpp"
#include "core/service_ids.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

  ExitState MuddleOutboundUpdateTask::run()
  {
    std::cout << "SENDING TO:" << target_ << std::endl;
    auto prom = client_ -> CallSpecificAddress(
      fetch::byte_array::FromBase64(byte_array::ConstByteArray(target_)),
      RPC_COLEARN,
      ColearnProtocol::RPC_COLEARN_UPDATE,
      type_name_,
      update_);
    prom -> Wait();
    return ExitState::COMPLETE;
  }

  bool MuddleOutboundUpdateTask::IsRunnable() const
  {
    return true;
  }
}
}
}

