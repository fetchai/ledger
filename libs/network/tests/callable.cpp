#include <iostream>

#include "core/serializers/byte_array_buffer.hpp"
#include "network/service/callable_class_member.hpp"
using namespace fetch::service;

class Foo
{
public:
  void Test(int a, int b, int c)
  {
    std::cout << a << " " << b << " " << c << std::endl;
  }
  void Blah(int n) {}
};

int main()
{
  Foo               class_instance;
  AbstractCallable *ac = new CallableClassMember<Foo, void(int, int, int), 1>(
      &class_instance, &Foo::Test);
  AbstractCallable *t2 =
      new CallableClassMember<Foo, void(int), 1>(&class_instance, &Foo::Blah);

  CallableClassMember<Foo, void(int, int, int), 1> &f =
      *((CallableClassMember<Foo, void(int, int, int), 1> *)ac);

  serializer_type args, ret;

  args << int(2) << int(4) << int(3);
  args.Seek(0);
  f(ret, args);

  int q = 9;

  CallableArgumentList extra;
  extra.PushArgument(&q);

  args.Seek(0);
  f(ret, extra, args);

  (*t2)(ret, extra, args);
}
