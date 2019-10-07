#pragma once

#include "oef-base/threading/Waitable.hpp"
#include <atomic>

template <class T>
class Future : public Waitable
{
public:
  Future()
    : Waitable()
  {}

  virtual ~Future()
  {}

  void set(const T &value)
  {
    value_.store(value);
    this->wake();
  }

  T get()
  {
    return value_.load();
  }

protected:
  std::atomic<T> value_;

private:
  Future(const Future &other) = delete;              // { copy(other); }
  Future &operator=(const Future &other)  = delete;  // { copy(other); return *this; }
  bool    operator==(const Future &other) = delete;  // const { return compare(other)==0; }
  bool    operator<(const Future &other)  = delete;  // const { return compare(other)==-1; }
};

template <class T>
class FutureComplexType : public Waitable
{
public:
  FutureComplexType()
    : Waitable()
  {}

  virtual ~FutureComplexType()
  {}

  void set(const T &value)
  {
    {
      std::lock_guard<std::mutex> lg(value_mutex_);
      value_ = value;
    }
    this->wake();
  }

  T get()
  {
    std::lock_guard<std::mutex> lg(value_mutex_);
    return value_;
  }

protected:
  T          value_;
  std::mutex value_mutex_;

private:
  FutureComplexType(const FutureComplexType &other) = delete;  // { copy(other); }
  FutureComplexType &operator                       =(const FutureComplexType &other) =
      delete;                                                // { copy(other); return *this; }
  bool operator==(const FutureComplexType &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const FutureComplexType &other)  = delete;  // const { return compare(other)==-1; }
};
