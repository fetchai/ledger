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

#include "Monitoring.hpp"

class Gauge
{
public:
  explicit Gauge(const char *name)
  {
    id = Monitoring::find(name);
  }
  virtual ~Gauge() = default;
  Gauge(const Gauge &other)
  {
    copy(other);
  }
  Gauge &operator=(const Gauge &other)
  {
    copy(other);
    return *this;
  }
  Gauge &operator=(std::size_t x)
  {
    Monitoring::set(id, x);
    return *this;
  }

  std::size_t get() const
  {
    return Monitoring::get(id);
  }

  Gauge &set(std::size_t x)
  {
    Monitoring::set(id, x);
    return *this;
  }
  Gauge &add(std::size_t x)
  {
    Monitoring::add(id, x);
    return *this;
  }
  Gauge &sub(std::size_t x)
  {
    Monitoring::sub(id, x);
    return *this;
  }

  Gauge &max(std::size_t x)
  {
    Monitoring::max(id, x);
    return *this;
  }

  Gauge &operator+=(std::size_t x)
  {
    Monitoring::add(id, x);
    return *this;
  }
  Gauge &operator-=(std::size_t x)
  {
    Monitoring::sub(id, x);
    return *this;
  }
  Gauge &operator++()
  {
    Monitoring::add(id, 1);
    return *this;
  }
  const Gauge operator++(int)
  {
    Monitoring::add(id, 1);
    return *this;
  }
  Gauge &operator--()
  {
    Monitoring::sub(id, 1);
    return *this;
  }
  const Gauge operator--(int)
  {
    Monitoring::sub(id, 1);
    return *this;
  }

protected:
  std::size_t id;
  void        copy(const Gauge &other)
  {
    id = other.id;
  }

private:
  bool operator==(const Gauge &other) = delete;
  bool operator<(const Gauge &other)  = delete;
};
