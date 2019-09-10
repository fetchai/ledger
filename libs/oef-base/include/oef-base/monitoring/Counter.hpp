#pragma once

#include "Monitoring.hpp"

#include <iostream>

class Counter
{
public:
  Counter(const char *name)
  {
    id = Monitoring::find(name);
  }
  Counter(const std::string &name)
  {
    id = Monitoring::find(name.c_str());
  }
  virtual ~Counter()
  {
  }
  Counter(const Counter &other) { copy(other); }
  Counter &operator=(const Counter &other) { copy(other); return *this; }

  Counter& add(std::size_t x) { Monitoring::add(id, x); return *this; }
  std::size_t get() const { return Monitoring::get(id); }

  Counter& operator+=(std::size_t x) { Monitoring::add(id, x); return *this; }
  Counter& operator++() { Monitoring::add(id, 1); return *this; }
  Counter  operator++(int) { Monitoring::add(id, 1); return *this; }
protected:
  std::size_t id = 0;
  void copy(const Counter &other)
  {
    id = other.id;
  }
private:
  bool operator==(const Counter &other) = delete;
  bool operator<(const Counter &other) = delete;
};
