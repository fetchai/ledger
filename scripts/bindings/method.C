#include<cstdint>
namespace fetch {
namespace Baz {

template< class T = int, typename U = T, int N = 21 >
class Foo
{
public:
  typedef int int_type;

  Foo();
  Foo(int const &x)
  {
  }

  Foo(Foo const&other) = delete;


  /* @brief Equal op??

     Invoke it while having a pint ...

   **/
  bool operator==(Foo const &other) = delete;

  bool operator==(Foo &&other)
  {

  }

  //< Some comment?
  float operator*=(float const &other)
  {

  }

  double operator*=(double const &other)
  {

  }

  float operator*(float const &other)
  {

  }

  double operator*(double const &other)
  {

  }


  virtual void Xx() = 0;


  /* This is the bar function.

     Invoke it while having a pint ...

   **/
  int_type bar(int_type input)
  {

  }

  int_type bar(uint64_t input) const
  {

  }

protected:
  void another(int input, double & output);
private:
  void ilanother(int input, double & output) {
  }


  template< typename S >
  void X(S t) { }
};


class Blah : public Foo< uint32_t >
{
public:
  Blah();
  Blah(int const &x)
  {
  }


  void bar(int input)
  {

  }

  void bar(uint64_t input)
  {

  }


  void another(int input, double & output);
  void ilanother(int input, double & output) {
  }


  template< typename T >
  void X(T t) { }
};


};
};
