#pragma once


namespace fetch
{
namespace generics
{

template<class TYPE, class MUTEXTYPE>
class Locked
{
public:
  Locked(MUTEXTYPE &m, TYPE object)
    : lock(m),
      target(object)
  {
  }

  Locked(Locked &&other)
    : lock(std::move(other.lock)),
      target(std::move(other.target))
  {
  }

  ~Locked()
  {
  }

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
  TYPE target;
};

}
}
