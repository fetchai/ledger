#include "serializer/typed_byte_array_buffer.hpp"
using namespace fetch::serializers;
std::ostream& operator<<(std::ostream& os, std::vector< int > const &v);
#include "unittest.hpp"



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

class NoCopyClass
{
public:

  NoCopyClass(){}

  NoCopyClass(int val) :
    classValue_{val} { }

  NoCopyClass(NoCopyClass &rhs)             = delete;
  NoCopyClass &operator=(NoCopyClass& rhs)  = delete;
  NoCopyClass(NoCopyClass &&rhs)            = delete;
  NoCopyClass &operator=(NoCopyClass&& rhs) = delete;

  int classValue_ = 0;
};

template <typename T>
inline void Serialize(T &serializer, NoCopyClass const &b)
{
  serializer << b.classValue_;
}

template <typename T>
inline void Deserialize(T &serializer, NoCopyClass &b)
{
  serializer >> b.classValue_;
}



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

      {
        TypedByte_ArrayBuffer buffer;
        std::vector<int> testVector{1,2,3,4};
        buffer << testVector;
        buffer.Seek(0);
        std::vector<int> answer;
        buffer >> answer;

        EXPECT(testVector == answer);
      }
    };
  };

  return 0;
}
