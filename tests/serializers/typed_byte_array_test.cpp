#include "serializer/typed_byte_array_buffer.hpp"
using namespace fetch::serializers;

#include "unittest.hpp"

class TestClass
{
public:

  TestClass(){}

  TestClass(int val) :
    classValue_{val} { }

  TestClass(TestClass &rhs)
  {
    std::cout << "Class copied" << std::endl;
    //classValue_ = 0;
    classValue_ = rhs.classValue_;
  }
  TestClass(TestClass &&rhs)            = delete;
  TestClass &operator=(TestClass& rhs)  = delete;
  TestClass &operator=(TestClass&& rhs) = delete;

  int classValue_;
};

template <typename T>
inline void Serialize(T &serializer, TestClass const &b)
{
  serializer << b.classValue_;
}

template <typename T>
inline void Deserialize(T &serializer, TestClass &b)
{
  serializer >> b.classValue_;
}


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
        TestClass test(99);
        buffer << test;
        buffer.Seek(0);
        TestClass result{};
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

    };
  };

  return 0;
}
