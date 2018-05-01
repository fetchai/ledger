#include <iostream>

#include "byte_array/referenced_byte_array.hpp"
#include "serializer/typed_byte_array_buffer.hpp"
using namespace fetch::byte_array;
using namespace fetch::serializers;

#include "unittest.hpp"

int main() {

  SCENARIO("Typed byte array serialization/deserialization") {

    SECTION("ensuring that subbyte_arrays come out correctly") {

      {
        TypedByte_ArrayBuffer buffer;
        buffer << 55;
        buffer.Seek(0);
        int answer;
        buffer >> answer;

        EXPECT(answer == 55);
      }

      {
        TypedByte_ArrayBuffer buffer;
        buffer << std::string("hello");
        buffer.Seek(0);
        std::string answer;
        buffer >> answer;

        EXPECT(answer.compare("hello") == 0);
      }

      {
        TypedByte_ArrayBuffer buffer;
        buffer << "Second hello";
        buffer.Seek(0);
        std::string answer;
        buffer >> answer;

        EXPECT(answer.compare("Second hello") == 0);
      }

    };
  };

  return 0;
}
