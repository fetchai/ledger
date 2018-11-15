#include "ledger/storage_unit/lane_controller.hpp"

namespace fetch {
namespace ledger {

void LaneController::WorkCycle()
  {
    UriSet remove;
    UriSet create;

    GeneratePeerDeltas(create, remove);

    FETCH_LOG_WARN(LOGGING_NAME, "WorkCycle:create:", create.size());
    FETCH_LOG_WARN(LOGGING_NAME, "WorkCycle:remove:", remove.size());

    for (auto &uri : create)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "WorkCycle:creating:", uri.ToString());
      muddle_->AddPeer(Uri(uri.ToString()));
    }

    for (auto &uri : create)
    {
      Address target_address;
      if (muddle_->GetOutgoingConnectionAddress(uri, target_address))
      {
        peer_connections_[uri] = target_address;
      }
    }
  }

}
}
