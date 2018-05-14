#include "serializer/typed_byte_array_buffer.hpp"
using namespace fetch::serializers;
std::ostream& operator<<(std::ostream& os, std::vector< int > const &v);
#include "unittest.hpp"
#include "../tests/include/helper_functions.hpp"

std::ostream& operator<<(std::ostream& os, std::vector< int > const &v)
{
  bool first = true;
  os << "[";
  for(auto const &e : v) {
    if(!first) os << ", ";
    os << e;
    first = false;
  }
  os << "]";

  return os;
}

using namespace fetch::serializers;
using namespace fetch::common;

int main() {

  SCENARIO("Typed byte array serialization/deserialization") {

    SECTION("ensuring that ser/deser is correct") {

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
        buffer << uint8_t(0);
        buffer << uint8_t(1);
        buffer << uint8_t(0xff);
        buffer << uint8_t(0xfe);
        buffer.Seek(0);

        uint8_t array[4];

        buffer >> array[0];
        buffer >> array[1];
        buffer >> array[2];
        buffer >> array[3];

        EXPECT(uint32_t(array[0]) == 0);
        EXPECT(uint32_t(array[1]) == 1);
        EXPECT(uint32_t(array[2]) == 0xff);
        EXPECT(uint32_t(array[3]) == 0xfe);
      }

      {
        TypedByte_ArrayBuffer buffer;
        buffer << uint16_t(0);
        buffer << uint16_t(1);
        buffer << uint16_t(0xffff);
        buffer << uint16_t(0xfffe);
        buffer.Seek(0);

        uint16_t array[4];

        buffer >> array[0];
        buffer >> array[1];
        buffer >> array[2];
        buffer >> array[3];

        EXPECT(uint32_t(array[0]) == 0);
        EXPECT(uint32_t(array[1]) == 1);
        EXPECT(uint32_t(array[2]) == 0xffff);
        EXPECT(uint32_t(array[3]) == 0xfffe);
      }

      {
        TypedByte_ArrayBuffer buffer;
        buffer << int(-1);
        buffer << int(0);
        buffer << int(0xffff);
        buffer << int(0xfffe);
        buffer.Seek(0);

        int array[4];

        buffer >> array[0];
        buffer >> array[1];
        buffer >> array[2];
        buffer >> array[3];

        EXPECT(array[0] == -1);
        EXPECT(array[1] == 0);
        EXPECT(array[2] == 0xffff);
        EXPECT(array[3] == 0xfffe);
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

        NoCopyClass test(99);
        buffer << test;
        buffer.Seek(0);
        NoCopyClass result{};
        buffer >> result;

        EXPECT(result.classValue_ == 99);
      }

      {
        TypedByte_ArrayBuffer buffer;
        buffer << "Second hello";
        buffer.Seek(0);
        std::string answer;
        buffer >> answer;

        EXPECT(answer.compare("Second hello") == 0);
      }

      {
        TypedByte_ArrayBuffer buffer;
        buffer << "Second hello";
        buffer.Seek(0);
        std::string answer;
        buffer >> answer;

        EXPECT(answer.compare("Second hello") == 0);
      }

      {
        TypedByte_ArrayBuffer buffer;
        bool testBool = true;
        buffer << testBool;
        buffer.Seek(0);
        bool answer;
        buffer >> answer;

        EXPECT(testBool == answer);
      }

      {
        TypedByte_ArrayBuffer buffer;
        bool testBool = false;
        buffer << testBool;
        buffer.Seek(0);
        bool answer;
        buffer >> answer;

        EXPECT(testBool == answer);
      }

      /*
      {
        TypedByte_ArrayBuffer buffer;
        std::vector<int> testVector{1,2,3,4};
        buffer << testVector;
        buffer.Seek(0);
        std::vector<int> answer;
        buffer >> answer;

        EXPECT(testVector != answer);
      } */
    };
  };

  return 0;
}
