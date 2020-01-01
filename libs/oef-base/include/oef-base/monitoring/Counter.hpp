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

#include <iostream>

class Counter
{
public:
  explicit Counter(char const *name)
  {
    id = Monitoring::find(name);
  }
  explicit Counter(std::string const &name)
  {
    id = Monitoring::find(name);
  }
  virtual ~Counter() = default;
  Counter(const Counter &other)
  {
    copy(other);
  }
  Counter &operator=(const Counter &other)
  {
    copy(other);
    return *this;
  }

  Counter &add(std::size_t x)
  {
    Monitoring::add(id, x);
    return *this;
  }
  std::size_t get() const
  {
    return Monitoring::get(id);
  }

  Counter &operator+=(std::size_t x)
  {
    Monitoring::add(id, x);
    return *this;
  }
  Counter &operator++()
  {
    Monitoring::add(id, 1);
    return *this;
  }
  const Counter operator++(int)
  {
    Monitoring::add(id, 1);
    return *this;
  }

protected:
  std::size_t id = 0;
  void        copy(const Counter &other)
  {
    id = other.id;
  }

private:
  bool operator==(const Counter &other) = delete;
  bool operator<(const Counter &other)  = delete;
};
