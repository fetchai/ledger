#include "dmlf/colearn/muddle_message_handler.hpp"
#include <unistd.h>

#include "dmlf/colearn/muddle_learner_networker_impl.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

  MuddleMessageHandler::MuddleMessageHandler(MuddleLearnerNetworkerImpl &impl, MuddlePtr muddle)
    : impl_(impl)
    , muddle_(muddle)
  {
  }

  MuddleMessageHandler::~MuddleMessageHandler()
  {
  }

  bool MuddleMessageHandler::supplyUpdate(CallContext const &/*context*/, const std::string &type, Bytes const &msg)
  {
    // TODO, create a message handling task at this point to do the indexing /evalution.

    // For now, directly add message.
    impl_ . NewMessage(type, msg);

    return true;
  }
}
}
}
