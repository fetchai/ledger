#pragma once

#include "Monitoring.hpp"

class Gauge
{
public:
  Gauge(const char *name)
  {
    id = Monitoring::find(name);
  }
  virtual ~Gauge()
  {
  }
  Gauge(const Gauge &other) { copy(other); }
  Gauge &operator=(const Gauge &other) { copy(other); return *this; }
  Gauge &operator=(std::size_t x) { Monitoring::set(id, x); return *this; }

  std::size_t get() const { return Monitoring::get(id); }

  Gauge& set(std::size_t x) { Monitoring::set(id, x); return *this; }
  Gauge& add(std::size_t x) { Monitoring::add(id, x); return *this; }
  Gauge& sub(std::size_t x) { Monitoring::sub(id, x); return *this; }

  Gauge& max(std::size_t x) { Monitoring::max(id, x); return *this; }

  Gauge& operator+=(std::size_t x) { Monitoring::add(id, x); return *this; }
  Gauge& operator-=(std::size_t x) { Monitoring::sub(id, x); return *this; }
  Gauge& operator++()              { Monitoring::add(id, 1); return *this; }
  Gauge  operator++(int)           { Monitoring::add(id, 1); return *this; }
  Gauge& operator--()              { Monitoring::sub(id, 1); return *this; }
  Gauge  operator--(int)           { Monitoring::sub(id, 1); return *this; }
protected:
  std::size_t id;
  void copy(const Gauge &other)
  {
    id = other.id;
  }
private:
  bool operator==(const Gauge &other) = delete;
  bool operator<(const Gauge &other) = delete;
};
