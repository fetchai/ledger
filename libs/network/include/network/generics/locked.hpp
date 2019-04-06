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

namespace fetch {
namespace generics {

template <class TYPE, class MUTEXTYPE>
class Locked
{
public:
  Locked(MUTEXTYPE &m, TYPE object)
    : lock(m)
    , target(object)
  {}

  Locked(Locked &&other)
    : lock(std::move(other.lock))
    , target(std::move(other.target))
  {}

  ~Locked()
  {}

  operator TYPE()
  {
    return target;
  }

  operator const TYPE() const
  {
    return target;
  }

  const TYPE operator->() const
  {
    return target;
  }

  TYPE operator->()
  {
    return target;
  }

private:
  std::unique_lock<MUTEXTYPE> lock;
  TYPE                        target;
};

}  // namespace generics
}  // namespace fetch
