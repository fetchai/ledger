//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/service_ids.hpp"
#include "ledger/protocols/notarisation_protocol.hpp"
#include "ledger/protocols/notarisation_service.hpp"

namespace fetch {
namespace ledger {

NotarisationServiceProtocol::NotarisationServiceProtocol(NotarisationService &service)
  : service_(service)
{
  this->Expose(GET_NOTARISATIONS, &service_, &NotarisationService::GetNotarisations);
}

}  // namespace ledger
}  // namespace fetch
