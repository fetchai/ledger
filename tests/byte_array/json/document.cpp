#include<iostream>
#include<byte_array/json/document.hpp>
using namespace fetch::byte_array;

#include"unittest.hpp"

int main() {
  SCENARIO("") {
    ReferencedByteArray doc_content = R"({
  "x": { 
    "y": [1,2,3],
    "z": null
  }
}
)" ;

    JSONDocument doc;
    doc.Parse("test.file", doc_content);
    
  };
  
  /*
  for(auto &t : test) {
    std::cout << t.filename() << " line " << t.line() << ", char " << t.character() << std::endl;
    std::cout << t.type() << " " << t.size() << " " << t << std::endl;
  }
  */
  return 0;
}
