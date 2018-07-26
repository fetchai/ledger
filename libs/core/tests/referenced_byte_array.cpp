#include <iostream>

#include "core/byte_array/byte_array.hpp"
using namespace fetch::byte_array;

#include "testing/unittest.hpp"

int main()
{
  SCENARIO("Class members of ByteArray")
  {
    char const *base    = "hello world";
    std::string basecpp = base;
    ByteArray   str(base);

    EXPECT(str == base);
    EXPECT(str == basecpp);

    SECTION("ensuring that subbyte_arrays come out correctly")
    {
      EXPECT(str.SubArray(0, 5) == "hello");
      EXPECT(str.SubArray(0, 5) == basecpp.substr(0, 5));

      EXPECT(str.SubArray(6, str.size() - 6) == "world");
      EXPECT(str.SubArray(6, str.size() - 6) == basecpp.substr(6, basecpp.size() - 6));
    };

    SECTION("testing that referencing persists after subbyte_arraying ")
    {
      ByteArray copy = str.SubArray(6, str.size() - 6);
      copy[0]        = 'k';
      copy[1]        = 'i';
      copy[2]        = 't';
      copy[3]        = 't';
      copy[4]        = 'y';

      EXPECT(str == "hello kitty");
    };

    // Even though the previous section copied that byte_array object, the
    // underlying
    // data is still referenced.
    EXPECT(str == "hello kitty");

    SECTION("testing that referencing vanishes after copying ")
    {
      ByteArray copy = str.Copy().SubArray(6, str.size() - 6);
      copy[0]        = 'Z';
      copy[1]        = 'i';
      copy[2]        = 'p';
      copy[3]        = 'p';
      copy[4]        = 'y';

      EXPECT(copy == "Zippy");
      EXPECT(str == "hello kitty");
    };

    SECTION("basic concat operations")
    {
      EXPECT((str + " kat") == "hello kitty kat");
      EXPECT(("Big " + str) == "Big hello kitty");
    };

    SECTION("size of loaded C strings")
    {
      EXPECT(ByteArray("any carnal pleas").size() == 16);
      EXPECT(ByteArray("any carnal pleasu").size() == 17);
      EXPECT(ByteArray("any carnal pleasur").size() == 18);
      EXPECT(ByteArray("any carnal pleasure").size() == 19);
      EXPECT(ByteArray("any carnal pleasure.").size() == 20);
    };
  };

  return 0;
}
