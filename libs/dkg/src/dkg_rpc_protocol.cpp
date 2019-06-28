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

#include "dkg/dkg_rpc_protocol.hpp"
#include "dkg/dkg_service.hpp"

namespace fetch {
namespace dkg {

DkgRpcProtocol::DkgRpcProtocol(DkgService &service)
  : service_{service}
{
  Expose(REGISTER, &service_, &DkgService::RegisterCabinetMember);
  Expose(REQUEST_SECRET, &service_, &DkgService::RequestSecretKey);
  Expose(SUBMIT_SIGNATURE, &service_, &DkgService::SubmitSignatureShare);
}

}  // namespace dkg
}  // namespace fetch
