#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <unordered_map>
#include <vector>
#include <functional>

namespace fetch {
namespace p2p {
namespace bayrank {

template <typename OBJECT_TYPE, typename IDENTITY>
class ObjectCache
{
public:
  using IdentityStore = std::vector<IDENTITY>;
  using Store         = std::unordered_map<OBJECT_TYPE, IdentityStore>;
  using Function      = std::function<void(IDENTITY const&)>;
  using Mutex         = mutex::Mutex;

  ObjectCache()                       = default;
  ObjectCache(const ObjectCache &rhs) = delete;
  ObjectCache(ObjectCache &&rhs)      = delete;
  ~ObjectCache()                      = default;

  // Operators
  ObjectCache operator=(const ObjectCache &rhs) = delete;
  ObjectCache operator=(ObjectCache &&rhs) = delete;


  void Add(OBJECT_TYPE const &object, IDENTITY const &identity)
  {
    FETCH_LOCK(mutex_);
    auto it = storage_.find(object);
    if (it!=storage_.end())
    {
      it->second.push_back(identity);
      return;
    }
    storage_.insert({object, IdentityStore{identity}});
  }

  void Remove(OBJECT_TYPE const &object)
  {
    storage_.erase(object);
  }

  bool Iterate(OBJECT_TYPE const &object, Function function)
  {
    FETCH_LOCK(mutex_);
    auto it = storage_.find(object);
    if (it==storage_.end())
    {
      return false;
    }
    for(auto const &identity : it->second)
    {
      function(identity);
    }
    return true;
  }

private:
  mutable Mutex mutex_{__LINE__, __FILE__};
  Store storage_;
};

} //namespace bayrank
} //namespace p2p
} //namespace fetch