#include "core/byte_array/tokenizer/tokenizer.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "core/byte_array/consumers.hpp"
#include "testing/unittest.hpp"

using namespace fetch::byte_array;

bool equals(Tokenizer const &tokenizer,
            std::vector<fetch::byte_array::ConstByteArray> const &ref) {
  if (ref.size() != tokenizer.size()) return false;
  for (std::size_t i = 0; i < ref.size(); ++i) {
    if (ref[i] != tokenizer[i]) return false;
  }

  return true;
}

bool equals(Tokenizer const &tokenizer, std::vector<int> const &ref) {
  if (ref.size() != tokenizer.size()) return false;
  for (std::size_t i = 0; i < ref.size(); ++i) {
    if (ref[i] != tokenizer[i].type()) return false;
  }

  return true;
}

int main(int argc, char **argv) {
  enum {
    E_INTEGER = 0,
    E_FLOATING_POINT = 1,
    E_STRING = 2,
    E_KEYWORD = 3,
    E_TOKEN = 4,
    E_WHITESPACE = 5,
    E_CATCH_ALL = 6
  };

  SCENARIO("Testing individual consumers") {
    SECTION("Any character") {
      Tokenizer test;
      test.AddConsumer(fetch::byte_array::consumers::AnyChar<E_CATCH_ALL>);
      std::string test_str;

      test_str = "hello world";
      EXPECT(test.Parse(test_str));
      EXPECT(test.size() == test_str.size());

      test_str = "12$1adf)(SD)S(*ASdf 09812 4e12";
      EXPECT(test.Parse(test_str));
      EXPECT(test.size() == test_str.size());

      test_str =
          "12$1adf)(SD)S(*ASdf 09812 4e12asd kalhsdak shd aopisfu q[wr "
          "iqrw'prkas'd;fkla;s'dfl;ak \"";
      EXPECT(test.Parse(test_str));
      EXPECT(test.size() == test_str.size());
    };

    SECTION("Any character") {
      Tokenizer test;
      test.AddConsumer(
          fetch::byte_array::consumers::NumberConsumer<E_INTEGER,
                                                       E_FLOATING_POINT>);
      test.AddConsumer(fetch::byte_array::consumers::AnyChar<E_CATCH_ALL>);

      std::string test_str;

      test_str = "93 -12.31 -12.e+3";
      EXPECT(test.Parse(test_str));
      EXPECT(equals(test, {"93", " ", "-12.31", " ", "-12.e+3"}));
      EXPECT(equals(test,
                    {E_INTEGER, E_CATCH_ALL, E_FLOATING_POINT, E_CATCH_ALL,
                     E_FLOATING_POINT}));
    };
  };

  // TODO: Add more tests
}
