#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"serializer/stl_types.hpp"
#include"serializer/byte_array_buffer.hpp"
using namespace fetch::serializers;
using namespace fetch::byte_array;

int main() {
  ReferencedByteArray str = "hello", str2 = "world";
  std::string nstr, nstr2;
  Byte_ArrayBuffer buffer;
  buffer << str << str2;
  buffer.Seek(0);
  buffer >> nstr >> nstr2;
  
  std::cout << nstr << std::endl << nstr2 << std::endl;
  return 0;
}
