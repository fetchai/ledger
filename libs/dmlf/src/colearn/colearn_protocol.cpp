#include "dmlf/colearn/colearn_protocol.hpp"
#include "dmlf/colearn/muddle_learner_networker_impl.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

  ColearnProtocol::ColearnProtocol(MuddleLearnerNetworkerImpl &exec)
  {
     ExposeWithClientContext(RPC_COLEARN_UPDATE, &exec, &MuddleLearnerNetworkerImpl::NetworkColearnUpdate);
  }

}
}
}
