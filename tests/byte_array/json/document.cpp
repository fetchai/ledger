#include<iostream>
#include<json/document.hpp>
using namespace fetch::json;
using namespace fetch::byte_array;

#include"unittest.hpp"

int main() {
  SCENARIO("") {
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

    JSONDocument doc;
    doc.Parse("test.file", doc_content);

    std::cout << doc.root() << std::endl;
    
  };
  
  /*
  for(auto &t : test) {
    std::cout << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
    std::cout << t.type() << " " << t.size() << " " << t << std::endl;
  }
  */
  return 0;
}
