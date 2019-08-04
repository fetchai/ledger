#include "beacon_setup_protocol.hpp"
#include "beacon_setup_service.hpp"
#include "core/service_ids.hpp"

namespace fetch {
namespace beacon {

BeaconSetupServiceProtocol::BeaconSetupServiceProtocol(BeaconSetupService &service)
  : service_(service)
{
  this->Expose(BeaconSetupService::SUBMIT_SHARE, &service_, &BeaconSetupService::SubmitShare);
}

}  // namespace beacon
}  // namespace fetch