#include <iostream>

namespace fetch {
namespace useless_namespace {
}

class helloWorld
{
public:
  using Xtype = int;
  typedef qqq = int;

  void WrongConstPosition(const &int x)
  {
    std::cout << x << std::endl;
  }

  void CorrectConstPosition(int const &x)
  {
    std::cout << x << std::endl;
  }

  void wrongName(int const &x)
  {
    std::cout << x << std::endl;
  }

  void wrong_nAme(int const &x)
  {
    std::cout << x << std::endl;
  }

  qqq test()
  {}
  Xtype test2()
  {}

  void correct_name(int const &x)
  {
    std::cout << x << std::endl;
  }

private:
  int var;
};

}  // namespace fetch