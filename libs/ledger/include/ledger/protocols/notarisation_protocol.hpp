#pragma once
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

#include "network/service/protocol.hpp"

namespace fetch {
namespace ledger {
class NotarisationService;

class NotarisationServiceProtocol : public service::Protocol
{
public:
  enum
  {
    GET_NOTARISATIONS = 1
  };

  // Construction / Destruction
  explicit NotarisationServiceProtocol(NotarisationService &service);

  NotarisationServiceProtocol(NotarisationServiceProtocol const &) = delete;
  NotarisationServiceProtocol(NotarisationServiceProtocol &&)      = delete;
  ~NotarisationServiceProtocol() override                          = default;

  // Operators
  NotarisationServiceProtocol &operator=(NotarisationServiceProtocol const &) = delete;
  NotarisationServiceProtocol &operator=(NotarisationServiceProtocol &&) = delete;

private:
  NotarisationService &service_;
};
}  // namespace ledger
}  // namespace fetch
