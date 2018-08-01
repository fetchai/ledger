#ifndef CALLBACKS_HPP
#define CALLBACKS_HPP

#include <iostream>
#include <string>

namespace fetch
{
namespace generics
{

template<typename FUNC>
class Callbacks
{
public:
  Callbacks(const Callbacks &rhs)            = delete;
  Callbacks(Callbacks &&rhs)                 = delete;
  Callbacks &operator=(const Callbacks &rhs)  = delete;
  Callbacks &operator=(Callbacks &&rhs)       = delete;
  bool operator==(const Callbacks &rhs) const = delete;
  bool operator<(const Callbacks &rhs) const  = delete;

  Callbacks &operator=(FUNC func)
  {
    callbacks_.push_back(func);
    return *this;
  }

  operator bool() const
  {
    return !callbacks_.empty();
  }

  template<class... U>
  void operator()(U&&... u)
  {
    for(auto func : callbacks_)
    {
      func(std::forward<U>(u)...);
    }
  }

  void clear(void)
  {
    callbacks_.clear();
  }

  explicit Callbacks()
  {
  }

  virtual ~Callbacks()
  {
  }

private:
  std::vector<FUNC> callbacks_;
};

}
}

#endif //CALLBACKS_HPP
