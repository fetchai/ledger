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
  },
  "y": -3.3
}
)" ;

    std::cout << doc_content << std::endl;

    JSONDocument doc;
    doc.Parse("test.file", doc_content);
    
    doc["a"] = 4;    
    std::cout << doc.root() << std::endl;

    std::cout << doc["y"].as_double() << std::endl;

    
    doc.Parse("hello", R"( {"thing": "tester", "list": [{"one": "me"}, {"two": "asdf"}]})");
    std::cout << doc.root() << std::endl << std::endl;
    std::cout << "doc[\"list\"] = ";    
    std::cout << doc["list"] << std::endl;
    std::cout << "List: "<<std::endl;
    
    for(auto &a: doc["list"].as_array()) {
      std::cout << a << std::endl;
      
    }
    
  };
  
  /*
  for(auto &t : test) {
    std::cout << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
    std::cout << t.type() << " " << t.size() << " " << t << std::endl;
  }
  */
  return 0;
}
