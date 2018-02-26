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

    std::cout << doc_content << std::endl;

    JSONDocument doc;
    doc.Parse("test.file", doc_content);

    doc["a"] = 4;
    std::cout << doc.root() << std::endl;

    doc.Parse("hello", "{\"resources\":[\"aasdasd\"],\"body\":\"asdasgagag\"}");
    
  };
  
  /*
  for(auto &t : test) {
    std::cout << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
    std::cout << t.type() << " " << t.size() << " " << t << std::endl;
  }
  */
  return 0;
}
