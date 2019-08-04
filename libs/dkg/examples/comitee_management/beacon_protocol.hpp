#pragma once
#include "core/service_ids.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace beacon {
class BeaconService;

class BeaconServiceProtocol : public service::Protocol
{
public:
  enum
  {
    SUBMIT_SIGNATURE_SHARE = 1
  };

  // Construction / Destruction
  explicit BeaconServiceProtocol(BeaconService &service);

  BeaconServiceProtocol(BeaconServiceProtocol const &) = delete;
  BeaconServiceProtocol(BeaconServiceProtocol &&)      = delete;
  ~BeaconServiceProtocol() override                    = default;

  // Operators
  BeaconServiceProtocol &operator=(BeaconServiceProtocol const &) = delete;
  BeaconServiceProtocol &operator=(BeaconServiceProtocol &&) = delete;

private:
  BeaconService &service_;
};
}  // namespace beacon
}  // namespace fetch