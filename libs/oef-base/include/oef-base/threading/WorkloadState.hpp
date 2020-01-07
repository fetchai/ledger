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

namespace fetch {
namespace oef {
namespace base {

enum class WorkloadProcessed
{
  COMPLETE     = 0,
  NOT_COMPLETE = 1,
  NOT_STARTED  = 2,
};

enum class WorkloadState
{
  START,
  RESUME,
};

extern const char *const workloadProcessedNames[];
}  // namespace base
}  // namespace oef
}  // namespace fetch
