#include "beacon/beacon_protocol.hpp"
#include "beacon/beacon_service.hpp"
#include "core/service_ids.hpp"

namespace fetch {
namespace beacon {

BeaconServiceProtocol::BeaconServiceProtocol(BeaconService &service)
  : service_(service)
{
  this->Expose(SUBMIT_SIGNATURE_SHARE, &service_, &BeaconService::SubmitSignatureShare);
}

}  // namespace beacon
}  // namespace fetch