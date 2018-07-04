#include "testing/unittest.hpp"
#include "ledger/identifier.hpp"

using fetch::ledger::Identifier;

int main() {

  SCENARIO("Parsing") {
    SECTION("Basic Checks") {
      Identifier id("foo.bar.baz");

      EXPECT(id.name() == "baz");
      EXPECT(id.name_space() == "foo.bar");
      EXPECT(id[0] == "foo");
      EXPECT(id[1] == "bar");
      EXPECT(id[2] == "baz");
    };
  };

  SCENARIO("Ancestry Checks") {
    SECTION("Direct Parent") {
      Identifier parent("foo");
      Identifier child("foo.bar");

      EXPECT(parent.IsParentTo(child));
      EXPECT(child.IsChildTo(parent));
      EXPECT(parent.IsDirectParentTo(child));
      EXPECT(child.IsDirectChildTo(parent));
      EXPECT(!parent.IsChildTo(child));
      EXPECT(!child.IsParentTo(parent));
    };

    SECTION("Indirect Parent") {
      Identifier parent("foo");
      Identifier child("foo.bar.baz");

      EXPECT(parent.IsParentTo(child));
      EXPECT(child.IsChildTo(parent));
      EXPECT(!parent.IsDirectParentTo(child));
      EXPECT(!child.IsDirectChildTo(parent));
      EXPECT(!parent.IsChildTo(child));
      EXPECT(!child.IsParentTo(parent));
    };

    SECTION("Child") {
      Identifier parent("foo.baz");
      Identifier child("foo.bar");

      EXPECT(!parent.IsParentTo(child));
      EXPECT(!child.IsChildTo(parent));
      EXPECT(!child.IsParentTo(parent));
      EXPECT(!parent.IsChildTo(child));
    };
  };

  return 0;
}