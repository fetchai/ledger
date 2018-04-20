#include"service/callable_class_member.hpp"
#include "serializer/byte_array_buffer.hpp"
using namespace fetch::service;

class Foo 
{
public:
  void Test(int a, int b, int c) 
  {
    std::cout << a << " " << b << " " << c << std::endl;
  }
  
};


int main() 
{
  Foo class_instance;  
  AbstractCallable *ac =  new CallableClassMember<Foo, void(int, int, int) >(&class_instance, &Foo::Test);  

  CallableClassMember<Foo, void(int, int, int) , 1> &f = *( (CallableClassMember<Foo, void(int, int, int) , 1>*)ac );
  

  serializer_type args, ret;
  
  args << int(2) << int(4) << int(3);
  args.Seek(0);

  int q = 9;  
  std::vector< void * > extra;
  extra.push_back( (void*) &q );  

  
  f(ret, extra, args);
  
  
}
