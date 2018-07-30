#include "core/script/variant.hpp"
#include <iostream>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
using namespace fetch::script;

#include "testing/unittest.hpp"

int main()
{
  std::cout << "Size of variant: " << sizeof(Variant) << std::endl;

  SCENARIO("Basic manipulation")
  {
    SECTION("Variant")
    {
      Variant x;
      x = 1;
      EXPECT(x.type() == fetch::script::VariantType::INTEGER);
      x = "Hello world";
      EXPECT(x.type() == fetch::script::VariantType::STRING);
      x = nullptr;
      EXPECT(x.type() == fetch::script::VariantType::NULL_VALUE);
      x = 4.21;
      EXPECT(x.type() == fetch::script::VariantType::FLOATING_POINT);
      x.MakeUndefined();
      EXPECT(x.type() == fetch::script::VariantType::UNDEFINED);
    };

    SECTION("Variant list")
    {
      VariantArray x(6);
      EXPECT(x.size() == 6);

      x[0] = 1.2;
      x[1] = "Hello world";
      x[2] = 2;
      x[3] = true;
      x[5] = nullptr;

      EXPECT(x[0].type() == fetch::script::VariantType::FLOATING_POINT);
      EXPECT(x[1].type() == fetch::script::VariantType::STRING);
      EXPECT(x[2].type() == fetch::script::VariantType::INTEGER);
      EXPECT(x[3].type() == fetch::script::VariantType::BOOLEAN);
      EXPECT(x[4].type() == fetch::script::VariantType::UNDEFINED);
      EXPECT(x[5].type() == fetch::script::VariantType::NULL_VALUE);
    };

    SECTION("Variant object")
    {
      Variant obj                 = Variant::Object();
      obj["numberOfTransactions"] = uint32_t(9);
      EXPECT(obj["numberOfTransactions"].type() == fetch::script::VariantType::INTEGER);
      EXPECT(obj["numberOfTransactions"].As<int>() == 9);

      obj["numberOfTransactions"] = "Hello world";
      std::cout << obj["numberOfTransactions"].type() << std::endl;
      obj["blah"]  = 9;
      obj["Hello"] = false;
      obj["XX"]    = nullptr;

      std::cout << obj["numberOfTransactions"].type() << std::endl;

      EXPECT(obj["numberOfTransactions"].type() == fetch::script::VariantType::STRING);
      EXPECT(obj["numberOfTransactions"].as_byte_array() == "Hello world");

      EXPECT(obj["blah"].type() == fetch::script::VariantType::INTEGER);
      EXPECT(obj["blah"].As<int>() == 9);

      EXPECT(obj["Hello"].type() == fetch::script::VariantType::BOOLEAN);
      EXPECT(obj["Hello"].As<bool>() == false);

      EXPECT(obj["XX"].type() == fetch::script::VariantType::NULL_VALUE);

      // Failing test
      Variant result                 = Variant::Object();
      result["numberOfTransactions"] = uint32_t(2);
      result["hash"]                 = "some_hash";

      std::ostringstream stream;
      stream << result;  // failing operation

      // Remove all spaces for string compare
      std::string           asString{stream.str()};
      std::string::iterator end_pos = std::remove(asString.begin(), asString.end(), ' ');
      asString.erase(end_pos, asString.end());

      std::cerr << asString << std::endl;

      EXPECT(asString.compare("{\"numberOfTransactions\":2,\"hash\":\"some_hash\"}") == 0);
    };

    SECTION("Nested variants")
    {
      Variant x;
      x.MakeArray(2);
      x[0].MakeArray(3);
      x[0][0] = 1;
      x[0][1] = 3;
      x[0][2] = 7;
      x[1]    = 1.23e-6;

      EXPECT(x.type() == fetch::script::VariantType::ARRAY);
      EXPECT(x[0].type() == fetch::script::VariantType::ARRAY);
      EXPECT(x[0][0].type() == fetch::script::VariantType::INTEGER);
      EXPECT(x[0][1].type() == fetch::script::VariantType::INTEGER);
      EXPECT(x[0][2].type() == fetch::script::VariantType::INTEGER);
      EXPECT(x[1].type() == fetch::script::VariantType::FLOATING_POINT);

      std::cout << x << std::endl;
    };
  };

  SCENARIO("Streaming")
  {
    SECTION("Variant list")
    {
      VariantArray x(6);
      EXPECT(x.size() == 6);

      x[0] = 1.2;
      x[1] = "Hello world";
      x[2] = 2;
      x[3] = true;
      x[5] = nullptr;

      std::stringstream ss;
      ss.str("");
      ss << x;
      EXPECT(ss.str() == "[1.2, \"Hello world\", 2, true, (undefined), null]");
    };

    SECTION("Nested variants")
    {
      Variant x;

      x.MakeArray(2);
      x[0].MakeArray(3);
      x[0][0] = 1;
      x[0][1] = 3;
      x[0][2] = 7;
      x[1]    = 1.23;

      std::stringstream ss;
      ss.str("");
      ss << x;

      EXPECT(ss.str() == "[[1, 3, 7], 1.23]");
    };
  };

  return 0;
}
