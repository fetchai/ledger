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

#include "core/mutex.hpp"
#include "muddle/address.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace muddle {

class Muddle;

class MuddleRegistry
{
public:
  using WeakMuddlePtr = std::weak_ptr<Muddle>;
  using MuddleMap     = std::unordered_map<Muddle const *, WeakMuddlePtr>;

  static MuddleRegistry &Instance();

  MuddleRegistry()  = default;
  ~MuddleRegistry() = default;

  void Register(WeakMuddlePtr muddle);
  void Unregister(Muddle const *muddle);

  MuddleMap GetMap() const;

private:
  mutable Mutex lock_;
  MuddleMap     map_;
};

}  // namespace muddle
}  // namespace fetch
