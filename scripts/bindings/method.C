namespace Baz {
class Foo
{
public:
  Foo();
  Foo(int x);  

  void bar(int input);

  void another(int input, double & output);
  void ilanother(int input, double & output) {

  }
  template< typename T >
  void X(T t) { }
};
};

void
Baz::Foo::bar(int input)
{
  input += 1;
}

void
Baz::Foo::another(int input, double & output)
{
  input += 1;
  output = input * 1.2345;
}
