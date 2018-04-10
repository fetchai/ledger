#include<iostream>
#include<script/variant.hpp>

#include<cassert>
using namespace fetch::script;

#include"unittest.hpp"

int main() {
  VariantList x(5);

  std::cout << x.size() << std::endl;
  x[0] = 1.2;
  x[1] = "Hello world";
  x[2] = 2;
  x[3] = "Blah blah";
  x[4] = 9;    
  std::cout << x[0].as_double() << std::endl;
  std::cout << x[1].as_byte_array() << std::endl;
  std::cout << x[2].as_int() << std::endl;

  VariantList y(x, 2, 3);
  std::cout << y.size() << std::endl;
  std::cout << y <<  std::endl;
  std::cout << y[1] <<  std::endl;
  std::cout << y[2] <<  std::endl;    
  /*
  SCENARIO("Basic operations") {
    Variant a(1.1234), b(49);
    Variant str("Hello world");
    Variant list({"Hello", 4.5, {2, "is the new black"} });

    
    Variant obj2 = Variant::Object({
        {"hello", 2},
        {
          "blah", {1,2,3}
        }
      });
    Variant obj1 = obj2;
    Variant obj(obj1);

    std::cout << obj << std::endl;

    
    fetch::byte_array::ByteArray base = "\"hello\"";
    fetch::byte_array::ByteArray name = base.SubArray(1, base.size() -2 );
    
    
    obj[name] =  5.5;
    obj["xx"] = 9;
    
    std::cout << obj[name] << std::endl;
//    std::cout << obj["blah"] << std::endl;    
    std::cout << obj["xx"] << std::endl;       
    std::cout << obj << std::endl;

    
    SECTION("Type compatibility") {
      EXPECT(a.type() == VariantType::FLOATING_POINT);
      EXPECT(a == 1.1234);
      EXPECT(a != 1);
      EXPECT(b.type() == VariantType::INTEGER);
      EXPECT(b == 49);
      EXPECT(b != 49.);
      
      EXPECT(str.type() == VariantType::BYTE_ARRAY);
      EXPECT(str == "Hello world");

      EXPECT(list.type() == VariantType::ARRAY);
    };

    SECTION("Modifications") {

      list = {"Hi", {2.2, "world"}, 'c', true};
      Variant list2 = list;

      list2[0] = str.Copy();
      
      EXPECT( list2[0].type() == VariantType::BYTE_ARRAY );
      
      // FIXME: Does not work
      str[1] = 'i';
      str[2] = ',';
      list2[1] = 'a';
      std::cout << list << std::endl;          
    };
  


  };

  */
  return 0;
}

