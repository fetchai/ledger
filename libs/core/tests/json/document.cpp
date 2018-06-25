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

int main(int argc, char **argv) {
  SCENARIO("Testing basic parsing") {
    SECTION("Nathan test") {
      ByteArray data =
          "{\"HTTPPort\": 8081, \"IP\": \"localhost\", \"TCPPort\": 9081}";
      JSONDocument test(data);
      std::cout << data << std::endl;
      std::cout << "---" << std::endl;
      std::cout << test.root() << std::endl;

      std::string IP_ = std::string(test["IP"].as_byte_array());
      uint32_t port_ = uint16_t(test["TCPPort"].as_int());

      std::cerr << "port is is " << port_ << std::endl;
      std::cerr << "IP is " << IP_ << " >>> " << test["IP"] << std::endl;
    };

    SECTION("Parsing and modification of document") {
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

      JSONDocument doc;
      doc.Parse(doc_content);

      std::stringstream ss;
      ss << doc.root();
      EXPECT(
          ss.str() ==
          R"({"a": 3, "x": {"y": [1, 2, 3], "z": null, "q": [], "hello world": {}}})");

      doc["a"] = 4;
      ss.str("");
      ss << doc.root();
      EXPECT(
          ss.str() ==
          R"({"a": 4, "x": {"y": [1, 2, 3], "z": null, "q": [], "hello world": {}}})");

      doc["x"]["y"][1] = 5;
      ss.str("");
      ss << doc.root();
      EXPECT(
          ss.str() ==
          R"({"a": 4, "x": {"y": [1, 5, 3], "z": null, "q": [], "hello world": {}}})");

      doc["x"]["z"] = fetch::script::Variant({1, 2, 3, 4, 5});
      ss.str("");
      ss << doc.root();
      EXPECT(
          ss.str() ==
          R"({"a": 4, "x": {"y": [1, 5, 3], "z": [1, 2, 3, 4, 5], "q": [], "hello world": {}}})");

      ss.str("");
      ss << doc["x"]["y"];
      EXPECT(ss.str() == "[1, 5, 3]")
    };

    SECTION("Type parsing") {
      ByteArray doc_content = R"({
  "a": 3,
  "b": 2.3e-2,
  "c": 2e+9,
  "d": "hello",
  "e": null,
  "f": true,
  "g": false
}
)";

      JSONDocument doc;
      doc.Parse(doc_content);
      EXPECT(doc["a"].type() == fetch::script::VariantType::INTEGER);
      EXPECT(doc["b"].type() == fetch::script::VariantType::FLOATING_POINT);
      EXPECT(doc["c"].type() == fetch::script::VariantType::FLOATING_POINT);
      EXPECT(doc["d"].type() == fetch::script::VariantType::STRING);
      EXPECT(doc["e"].type() == fetch::script::VariantType::NULL_VALUE);

      EXPECT(doc["f"].type() == fetch::script::VariantType::BOOLEAN);
      EXPECT(doc["g"].type() == fetch::script::VariantType::BOOLEAN);
    };

    SECTION("Parsing exeptions") {
      JSONDocument doc;
      EXPECT_EXCEPTION(doc.Parse("{"), fetch::json::JSONParseException);
      EXPECT_EXCEPTION(doc.Parse("{]"), fetch::json::JSONParseException);

      EXPECT_EXCEPTION(doc.Parse(R"(["a":"b"])"),
                       fetch::json::JSONParseException);
      EXPECT_EXCEPTION(doc.Parse(R"({"a": 2.fs})"),
                       fetch::json::JSONParseException);
      // TODO      EXPECT_EXCEPTION(doc.Parse(R"({"a":})"),
      // fetch::json::JSONParseException);
    };

    return 0;
  };
}
