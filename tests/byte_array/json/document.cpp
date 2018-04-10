#include<iostream>
#include<json/document.hpp>
using namespace fetch::json;
using namespace fetch::byte_array;

#include"unittest.hpp"
/*
struct Blah 
{
  int i,j;
  std::string foo;
};

void Serializer(T &t, Blah const&b) 
{
  t.WithProperty("i") << i;
  t.WithProperty("j") << j;
  t.WithProperty("foo") << foo;    
}
*/

int main() {
  SCENARIO("Testing basic parsing") {
    ByteArray doc_content = R"({
  "a": 3,
  "x": { 
    "y": [1,2,3],
    "z": null,
    "q": [],
    "hello world": {}
  }
}
)" ;
    doc_content = R"({"angle1":0.22,"angle2":{ },"name":"AEA_8080_0","searchText":"100"})";
    //std::cout << doc_content << std::endl;
    JSONDocument doc;      
    for(std::size_t i=0; i < 1000000; ++i) {

      doc.Parse(doc_content);
    }
//    std::cout << doc.root() << std::endl;
    
    /*
    doc["a"] = 4;
    std::cout << doc.root() << std::endl;

    std::cout << " -----======------- " << std::endl;
    doc.Parse(R"( {"thing": "tester", "list": [{"one": "me"}, {"two": "asdf"}] } )");

    std::cout << doc.root() << std::endl << std::endl;
    std::cout << "doc[\"list\"] = ";    
    std::cout << doc["list"] << std::endl;
    std::cout << "List: "<<std::endl;
    
    for(auto &a: doc["list"].as_array()) {
      std::cout << a << std::endl;
      
    }
    
  };
    */  
  /*
  for(auto &t : test) {
    std::cout << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
    std::cout << t.type() << " " << t.size() << " " << t << std::endl;
  }
  */
  return 0;
  };
  

}
