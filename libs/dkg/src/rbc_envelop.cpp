#include "dkg/rbc_envelop.hpp"
#include "core/logging.hpp"

namespace fetch {
namespace dkg {
namespace rbc {

constexpr char const *LOGGING_NAME   = "RBCMessage";

std::shared_ptr<RBCMessage> RBCEnvelop::getMessage() const {
    RBCSerializer serialiser{serialisedMessage_};
    switch (type_) {
        case MessageType::RBROADCAST:
            return std::make_shared<RBroadcast>(serialiser);
        case MessageType::RECHO:
            return std::make_shared<REcho>(serialiser);
        case MessageType::RREADY:
            return std::make_shared<RReady>(serialiser);
        case MessageType::RREQUEST:
            return std::make_shared<RRequest>(serialiser);
        case MessageType::RANSWER:
            return std::make_shared<RAnswer>(serialiser);
        default:
            FETCH_LOG_ERROR(LOGGING_NAME, "Can not process payload");
    }
}

}
}
}