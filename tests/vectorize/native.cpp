#include<iostream>
#include"vectorize/register.hpp"
#include<random/lfg.hpp>
using namespace fetch::vectorize;

template< typename T >
using NativeRegister = VectorRegister< T, InstructionSet::NO_VECTOR > ;

template< typename T >
using NativeMemoryProxy = VectorMemoryProxy< T, InstructionSet::NO_VECTOR > ;

template< typename T >
using NativeMemory = VectorMemory< T, InstructionSet::NO_VECTOR > ;

fetch::random::LinearCongruentialGenerator lcg;

#define ADD_TEST(OP, NAME) \
  template< typename T, bool integral = true >  \
void test_##NAME() {\
  T a;                                          \
 T b ;                                          \
 if(integral) { a = lcg(); b = lcg(); } \
 else { a = lcg.AsDouble(); b = lcg.AsDouble(); } \
  NativeRegister< T > A(a), B(b);\
  NativeRegister< T > C = A OP B;\
  T c = a OP b; \
  if( T(C) != c ) {\
    std::cout << T(C) << " != " << c << std::endl; \
    std::cout << "for "#NAME << " using "#OP << std::endl;      \
    exit(-1);\
  }\
}

ADD_TEST(*,multiply);
ADD_TEST(+,add);
ADD_TEST(-,subtract);
ADD_TEST(/,divide);
ADD_TEST(&,and);
ADD_TEST(|,or);
ADD_TEST(^,xor);
#undef ADD_TEST

void test_registers() {
  for(std::size_t i=0; i < 10000000; ++i) {
    test_multiply<int8_t>();
    test_multiply<int16_t>();
    test_multiply<int32_t>();
    test_multiply<int64_t>();

    test_multiply<uint8_t>();
    test_multiply<uint16_t>();
    test_multiply<uint32_t>();
    test_multiply<uint64_t>();

    test_multiply<double, false>();
    test_multiply<float, false>();
    test_multiply<double>();
    test_multiply<float>();

    test_add<int8_t>();
    test_add<int16_t>();
    test_add<int32_t>();
    test_add<int64_t>();

    test_add<uint8_t>();
    test_add<uint16_t>();
    test_add<uint32_t>();
    test_add<uint64_t>();

    test_add<double, false>();
    test_add<float, false>();
    test_add<double>();
    test_add<float>();

    test_subtract<int8_t>();
    test_subtract<int16_t>();
    test_subtract<int32_t>();
    test_subtract<int64_t>();

    test_subtract<uint8_t>();
    test_subtract<uint16_t>();
    test_subtract<uint32_t>();
    test_subtract<uint64_t>();

    test_subtract<double, false>();
    test_subtract<float, false>();
    test_subtract<double>();
    test_subtract<float>();


    test_divide<int8_t>();
    test_divide<int16_t>();
    test_divide<int32_t>();
    test_divide<int64_t>();

    test_divide<uint8_t>();
    test_divide<uint16_t>();
    test_divide<uint32_t>();
    test_divide<uint64_t>();

    test_divide<double, false>();
    test_divide<float, false>();
    test_divide<double>();
    test_divide<float>();

    test_and<int8_t>();
    test_and<int16_t>();
    test_and<int32_t>();
    test_and<int64_t>();

    test_and<uint8_t>();
    test_and<uint16_t>();
    test_and<uint32_t>();
    test_and<uint64_t>();

    test_or<int8_t>();
    test_or<int16_t>();
    test_or<int32_t>();
    test_or<int64_t>();

    test_or<uint8_t>();
    test_or<uint16_t>();
    test_or<uint32_t>();
    test_or<uint64_t>();

    test_xor<int8_t>();
    test_xor<int16_t>();
    test_xor<int32_t>();
    test_xor<int64_t>();

    test_xor<uint8_t>();
    test_xor<uint16_t>();
    test_xor<uint32_t>();
    test_xor<uint64_t>();

  }
}


#define ADD_TEST(OP, NAME)                      \
  template< typename T, bool integral = true >  \
void mtest_##NAME() {\
  T a;                                          \
  T b ;                                         \
  if(integral) { a = lcg(); b = lcg(); }          \
  else { a = lcg.AsDouble(); b = lcg.AsDouble(); }      \
  NativeRegister< T > A(a), B(b);                       \
  NativeRegister< T > C = A OP B;                       \
  T c = a OP b;                                         \
  if( T(C) != c ) {                                     \
    std::cout << T(C) << " != " << c << std::endl;              \
    std::cout << "for "#NAME << " using "#OP << std::endl;      \
      exit(-1);                                                 \
  }                                                             \
  }

ADD_TEST(*,multiply);
ADD_TEST(+,add);
ADD_TEST(-,subtract);
ADD_TEST(/,divide);
ADD_TEST(&,and);
ADD_TEST(|,or);
ADD_TEST(^,xor);

#undef ADD_TEST

template<typename T = uint32_t>
void test_memory_proxy_read() {
  T array[4];
  for(std::size_t i=0; i < 4;++i)
    array[i] = lcg();

  NativeMemoryProxy<T> x(array);

  if(T(x) != T(array[0])) {
    std::cout << "proxy test failed" << std::endl;
    exit(-1);
  }

  NativeRegister<T> y = NativeRegister<T>(x);

  if(T(y) != array[0]) {
    std::cout << "proxy cast failed" << std::endl;
    exit(-1);
  }  
}

template<typename T = uint32_t>
void test_memory_proxy_write() {
  T array[4] = {0};
  
  T val = lcg();
  NativeMemoryProxy<T> x(array);
  x = val;
  if(array[0] != val) {
    std::cout << "failed writing" << std::endl;
    exit(-1);
  }
}

/*
void interface() {
  T array1[8];
  T array2[8];
  T ret[8];

  Vecotorize v1( array1 ), v2( array2 ), vr( ret );
  for(std::size_t i=0; i < v1.size(); ++i)
    vr[i] = v1[i] * v2[i];
}
*/
int main() {
  //  test_registers() ;
  test_memory_proxy_read();
  test_memory_proxy_write();
  return 0;
}
