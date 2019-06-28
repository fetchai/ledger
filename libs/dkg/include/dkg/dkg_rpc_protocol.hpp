#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "dkg/dkg_rpc_serializers.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace dkg {

class DkgService;

class DkgRpcProtocol : public service::Protocol
{
public:

  enum
  {
    REGISTER,
    REQUEST_SECRET,
    SUBMIT_SIGNATURE,
  };

  // Construction / Destruction
  explicit DkgRpcProtocol(DkgService &service);
  DkgRpcProtocol(DkgRpcProtocol const &) = delete;
  DkgRpcProtocol(DkgRpcProtocol &&) = delete;
  ~DkgRpcProtocol() override = default;

  // Operators
  DkgRpcProtocol &operator=(DkgRpcProtocol const &) = delete;
  DkgRpcProtocol &operator=(DkgRpcProtocol &&) = delete;

private:

  DkgService &service_;
};

} // namespace dkg
} // namespace fetch
