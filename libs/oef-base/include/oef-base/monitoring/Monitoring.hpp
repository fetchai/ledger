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

#include <atomic>
#include <functional>
#include <memory>
#include <string>

// class MonitoringInner;

#include "oef-base/utils/BucketsOf.hpp"

class Monitoring
{
public:
  using IdType    = std::size_t;
  using CountType = std::size_t;
  using NameType  = std::string;

  using MonitoringInner = BucketsOf<std::atomic<std::size_t>, std::string, std::size_t, 256>;

  using ReportFunc = std::function<void(const std::string &name, std::size_t value)>;

  Monitoring();
  virtual ~Monitoring() = default;

  static IdType    find(const NameType &name);
  static void      add(IdType id, CountType delta);
  static void      sub(IdType id, CountType delta);
  static void      set(IdType id, CountType delta);
  static void      max(IdType id, CountType value);
  static CountType get(IdType id);

  void report(ReportFunc const &func);

  static MonitoringInner *inner;

protected:
private:
  Monitoring(const Monitoring &other) = delete;
  Monitoring &operator=(const Monitoring &other)  = delete;
  bool        operator==(const Monitoring &other) = delete;
  bool        operator<(const Monitoring &other)  = delete;
};
