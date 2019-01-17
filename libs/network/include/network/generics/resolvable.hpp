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

#include "network/service/promise.hpp"

namespace fetch {
namespace network {

/**
 * An interface which allows access to the underlying functionality of Promise
 */
class Resolvable
{
public:
  using State          = service::PromiseState;
  using PromiseCounter = service::PromiseCounter;

  Resolvable()          = default;
  virtual ~Resolvable() = default;

  virtual State          GetState() = 0;
  virtual PromiseCounter id() const = 0;
};

template <class RESULT>
class ResolvableTo : public Resolvable
{
public:
  using State          = service::PromiseState;
  using PromiseCounter = service::PromiseCounter;
  using Clock          = std::chrono::steady_clock;
  using Timepoint      = Clock::time_point;

  ResolvableTo()  = default;
  ~ResolvableTo() = default;

  ResolvableTo(ResolvableTo const &rhs) = default;

  // Operators
  ResolvableTo &operator=(ResolvableTo const &rhs) = default;
  ResolvableTo &operator=(ResolvableTo &&rhs) noexcept = default;

  virtual State GetState(Timepoint const & /*tp*/)
  {
    return GetState();
  }
  virtual State          GetState() = 0;
  virtual PromiseCounter id() const = 0;

  virtual RESULT Get() const = 0;
};

}  // namespace network
}  // namespace fetch
