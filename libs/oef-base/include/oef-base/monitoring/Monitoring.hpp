#pragma once

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
  virtual ~Monitoring();

  static IdType    find(const NameType &name);
  static void      add(IdType id, CountType delta);
  static void      sub(IdType id, CountType delta);
  static void      set(IdType id, CountType delta);
  static void      max(IdType id, CountType value);
  static CountType get(IdType id);

  void report(ReportFunc);

  static MonitoringInner *inner;

protected:
private:
  Monitoring(const Monitoring &other) = delete;
  Monitoring &operator=(const Monitoring &other)  = delete;
  bool        operator==(const Monitoring &other) = delete;
  bool        operator<(const Monitoring &other)  = delete;
};
