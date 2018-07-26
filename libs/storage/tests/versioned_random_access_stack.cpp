#include "storage/versioned_random_access_stack.hpp"
#include "core/random/lfg.hpp"
#include "testing/unittest.hpp"

#include <iostream>
#include <stack>
#include <vector>
using namespace fetch::storage;

int main()
{
#define TYPE uint64_t

  SCENARIO("random access stack is being created and manipulated")
  {
    VersionedRandomAccessStack<TYPE> stack;
    stack.New("versioned_random_access_stack_test_1.db",
              "versioned_random_access_stack_diff.db");
    VersionedRandomAccessStack<TYPE>::bookmark_type cp1, cp2, cp3;

    SECTION_REF("testing basic manipulation")
    {
      cp1 = stack.Commit();
      stack.Push(1);
      stack.Push(2);
      stack.Push(3);
      cp2 = stack.Commit();
      stack.Swap(1, 2);
      stack.Push(4);
      stack.Push(5);
      stack.Set(0, 9);
      cp3 = stack.Commit();
      stack.Push(6);
      stack.Push(7);
      stack.Push(9);
      stack.Pop();

      EXPECT(stack.Top() == 7);
      EXPECT(stack.Get(0) == 9);
      EXPECT(stack.Get(1) == 3);
      EXPECT(stack.Get(2) == 2);
    };

    SECTION_REF("testing revert 1")
    {
      stack.Revert(cp3);
      EXPECT(stack.Top() == 5);
      EXPECT(stack.Get(0) == 9);
      EXPECT(stack.Get(1) == 3);

      EXPECT(stack.Get(2) == 2);
    };

    SECTION_REF("testing revert 2")
    {
      stack.Revert(cp2);
      EXPECT(stack.Top() == 3);
      EXPECT(stack.Get(0) == 1);
      EXPECT(stack.Get(1) == 2);
      EXPECT(stack.Get(2) == 3);
    };

    SECTION_REF("testing revert 3")
    {
      stack.Revert(cp1);
      EXPECT(stack.empty());
    };

    SECTION_REF("testing refilling")
    {
      cp1 = stack.Commit();
      stack.Push(1);
      stack.Push(2);
      stack.Push(3);
      cp2 = stack.Commit();
      stack.Swap(1, 2);
      stack.Push(4);
      stack.Push(5);
      stack.Set(0, 9);
      cp3 = stack.Commit();
      stack.Push(6);
      stack.Push(7);
      stack.Push(9);
      stack.Pop();

      EXPECT(stack.Top() == 7);
      EXPECT(stack.Get(0) == 9);
      EXPECT(stack.Get(1) == 3);
      EXPECT(stack.Get(2) == 2);
    };

    SECTION_REF("testing revert 2")
    {
      stack.Revert(cp2);
      EXPECT(stack.Top() == 3);
      EXPECT(stack.Get(0) == 1);
      EXPECT(stack.Get(1) == 2);
      EXPECT(stack.Get(2) == 3);
    };
  };

  SCENARIO("storage of large objects")
  {
    fetch::random::LaggedFibonacciGenerator<> lfg;

    struct Element
    {
      int      a;
      uint8_t  b;
      uint64_t c;
      uint16_t d;
      bool     operator==(Element const &o) const
      {
        return ((a == o.a) && (b == o.b) && (c == o.c) && (d == o.d));
      }
    };
    VersionedRandomAccessStack<Element> stack;
    stack.New("versioned_random_access_stack_test_2.db",
              "versioned_random_access_stack_diff2.db");
    std::vector<Element> reference;

    auto newElement = [&stack, &reference, &lfg]() -> Element {
      Element e;
      e.a = int(lfg());
      e.b = uint8_t(lfg());
      e.c = uint64_t(lfg());
      e.d = uint16_t(lfg());
      stack.Push(e);
      reference.push_back(e);
      return e;
    };

    bool all_equal = true;
    for (std::size_t i = 1; i < 20; ++i)
    {
      if ((i % 4) == 0) stack.Commit();
      newElement();
      all_equal &= (stack.Top() == reference.back());
    }
    CHECK("all top elements are equal during push", all_equal);

    all_equal = true;
    for (std::size_t i = 0; i < reference.size(); ++i)
    {
      all_equal &= (stack.Get(i) == reference[i]);
    }
    CHECK("all top elements are equal for random access", all_equal);
  };
  return 0;
}
