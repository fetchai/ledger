#ifndef LIFE_TRACKER_HPP
#define LIFE_TRACKER_HPP

#include <iostream>
#include <string>

template<class TARGET>
class LambdaThis
{
  using mutex_type = std::mutex;
  using lock_type = std::unique_lock<std::mutex>;
public:
  LambdaThis(TARGET *target)
    : original_(true)
    , master_(new MasterRecord(target))
  {
  }

  LambdaThis(const LambdaThis &other)
    : original_(false)
  {
    master_ = other.master_;
    master_ -> count.fetch++;
  }

  ~LambdaThis()
  {
    if (original)
    {
      lock_type lock(master_ -> mutex_);
      master_ -> target_ = 0;
    }
    master_ -> count--;
    if (!master -> count)
    {
      lock_type lock(master_ -> mutex_);
      delete master;
    }
  }

  Locked lock()
  {
    return Locked(master);
  }

  class Locked
  {
    friend class LambdaThis;
  protected:
    Locked(MasterRecord *master)
      : target_(master -> target)
      , lock_(master -> mutex)
    {
    }
  public:
    ~Locked()
    {
    }

    Locked(Locked &&other)
      : target_(other.target_)
      , lock(std::move(other.lock_)
    {
    }

    operator bool() const
    {
      return target_;
    }

    TARGET *operator->()
    {
      return target_;
    }
  protected:
    TARGET *target_;
    lock_type lock_;
  };

private:

  class MasterRecord
  {
  public:
    MasterRecord(TARGET *target)
      : target_(target)
      , count(0)
    {
    }
    ~MasterRecord()
    {
    }
  protected:
    TARGET *target_;
    std::atomic<unsigned int> count;
    mutex_type mutex_;
  }

  bool original_;
  MasterRecord master_;
}
