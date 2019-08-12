#pragma once
#include "core/service_ids.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace beacon {
class BeaconSetupService;
class BeaconSetupServiceProtocol : public service::Protocol
{
public:
  // Construction / Destruction
  explicit BeaconSetupServiceProtocol(BeaconSetupService &service);

  BeaconSetupServiceProtocol(BeaconSetupServiceProtocol const &) = delete;
  BeaconSetupServiceProtocol(BeaconSetupServiceProtocol &&)      = delete;
  ~BeaconSetupServiceProtocol() override                         = default;

  // Operators
  BeaconSetupServiceProtocol &operator=(BeaconSetupServiceProtocol const &) = delete;
  BeaconSetupServiceProtocol &operator=(BeaconSetupServiceProtocol &&) = delete;

private:
  BeaconSetupService &service_;
};
}  // namespace beacon
}  // namespace fetch