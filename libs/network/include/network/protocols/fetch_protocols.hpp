#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

namespace fetch {
namespace protocols {

struct FetchProtocols
{

  enum
  {
    SWARM             = 2,
    MAIN_CHAIN        = 3,
    EXECUTION_MANAGER = 4,
    EXECUTOR          = 5,
    STATE_DATABASE    = 6
  };
};

}  // namespace protocols
}  // namespace fetch
