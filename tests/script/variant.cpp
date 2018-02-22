#include<iostream>
#include<script/variant.hpp>

#include<cassert>
using namespace fetch::script;

#include"unittest.hpp"

int main() {
  SCENARIO("Basic operations") {
    Variant a(1.1234), b(49);
    Variant str("Hello world");
    Variant list({"Hello", 4.5, {2, "is the new black"} });

    
    Variant obj = Variant::Object( 
      {
        {"hello", 2},
        {
          "blah", {1,2,3}
        },
        {
          2
            }
      });

    obj["hello"] =  5.5;
    
    std::cout << obj["hello"] << std::endl;
    std::cout << obj["blah"] << std::endl;    
      
    
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


  return 0;
}

