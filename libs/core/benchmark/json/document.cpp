//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <iostream>

#include "core/json/document.hpp"

using namespace fetch::json;
using namespace fetch::byte_array;

#include "testing/unittest.hpp"
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

int main(int argc, char **argv)
{
  SCENARIO("Testing basic parsing")
  {
    ByteArray doc_content = R"({
  "a": 3,
  "x": { 
    "y": [1,2,3],
    "z": null,
    "q": [],
    "hello world": {}
  }
}
)";
    /*
    int fsize = 0;
    FILE *fp;
    fp = fopen(argv[1], "r");
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    doc_content.Resize( fsize);
    fread(reinterpret_cast< char *>(doc_content.pointer()), 1, fsize, fp);
    fclose(fp);
    */

    // std::cout << doc_content << std::endl;
    JSONDocument doc;
    for (std::size_t i = 0; i < 1; ++i)
    {
      doc.Parse(doc_content);
    }

    std::cout << doc.root() << std::endl;

    doc["a"] = 4;
    std::cout << doc.root() << std::endl;
    /*
    std::cout << " -----======------- " << std::endl;
    doc.Parse(R"( {"thing": "tester", "list": [{"one": "me"}, {"two": "asdf"}] }
  )");

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
      std::cout << t.filename() << " line " << t.line() << ", char " <<
    t.character() << std::endl;
      std::cout << t.type() << " " << t.size() << " " << t << std::endl;
    }
    */
    return 0;
  };
}
