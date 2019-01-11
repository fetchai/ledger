//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/random/lfg.hpp"
#include "storage/mmap_random_access_stack.hpp"

#include <gtest/gtest.h>
#include <stack>

using namespace fetch::storage;

class TestClass
{
public:
  uint64_t value1 = 0;
  uint8_t  value2 = 0;

  bool operator==(TestClass const &rhs)
  {
    return value1 == rhs.value1 && value2 == rhs.value2;
  }
};

TEST(mmap_random_access_stack , MAX_objects)
{
  constexpr uint64_t                        testSize = 100000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  std::vector<TestClass>                    reference;

  {
    MMapRandomAccessStack<TestClass , uint64_t , 512>          stack;
    stack.New("test_mmap.db");
    EXPECT_TRUE(stack.is_open());
    for (uint64_t i = 0; i < testSize; ++i)
    {
      uint64_t  random = lfg();
      TestClass temp;
      temp.value1 = random;
      temp.value2 = random & 0xFF;
      stack.Push(temp);
      
      reference.push_back(temp);
      ASSERT_TRUE(stack.Top() == reference[i]) << "Stack did not match reference stack at index " << i;
    }
  }

  {
    MMapRandomAccessStack<TestClass , uint64_t , 1024>          stack;
    stack.New("test_mmap.db");
    EXPECT_TRUE(stack.is_open());
    for (uint64_t i = 0; i < testSize; ++i)
    {
      stack.Push(reference[i]);
      ASSERT_TRUE(stack.Top() == reference[i]) << "Stack did not match reference stack at index " << i;
    }
  }
}

TEST(mmap_random_access_stack, basic_functionality)
{
  constexpr uint64_t                        testSize = 100000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  MMapRandomAccessStack<TestClass>          stack;
  std::vector<TestClass>                    reference;

  stack.New("test_mmap.db");
  EXPECT_TRUE(stack.is_open());

  // Test push/top
  for (uint64_t i = 0; i < testSize; ++i)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;
    stack.Push(temp);
    
    reference.push_back(temp);
    TestClass temp2 = stack.Top();
    ASSERT_TRUE(temp2 == reference[i]) << "Stack did not match reference stack at index " << i;
  }
  std::cout << "\n testing push and top completed\n";
  // Test index
  {
    ASSERT_TRUE(stack.size() == reference.size());

    for (uint64_t i = 0; i < testSize; ++i)
    {
      TestClass temp;
      stack.Get(i, temp);
      ASSERT_TRUE(temp == reference[i]) << "at index" << i << " temp value1 = " << temp.value1
                                        << " reference value =" << reference[i].value1 << std::endl;
    }
  }
  std::cout << "\n testing index completed\n";
  // Test setting
  for (uint64_t i = 0; i < testSize; ++i)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;

    stack.Set(i, temp);
    reference[i] = temp;
  }
  std::cout << "\n testing setting completed\n";
  // Test swapping
  for (std::size_t i = 0; i < 100; ++i)
  {
    uint64_t pos1 = lfg() % testSize;
    uint64_t pos2 = lfg() % testSize;

    TestClass a;
    stack.Get(pos1, a);

    TestClass b;
    stack.Get(pos2, b);

    stack.Swap(pos1, pos2);

    {
      TestClass c;
      stack.Get(pos1, c);

      ASSERT_TRUE(c == b) << "Stack swap test failed, iteration " << i;
    }

    {
      TestClass c;
      stack.Get(pos2, c);

      ASSERT_TRUE(c == a) << "Stack swap test failed, iteration " << i;
    }
  }
  std::cout << "\n testing swap completed\n";
  // Pop items off the stack
  for (std::size_t i = 0; i < testSize; ++i)
  {
    stack.Pop();
  }

  ASSERT_TRUE(stack.size() == 0);
  ASSERT_TRUE(stack.empty() == true);
  std::cout << "\n testing pop completed\n";
}
TEST(mmap_random_access_stack, get_bulk)
{
  constexpr uint64_t                        testSize = 1000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  MMapRandomAccessStack<TestClass>          stack;
  std::vector<TestClass>                    reference;

  stack.New("test_mmap.db");
  EXPECT_TRUE(stack.is_open());

  for (uint64_t i = 0; i < testSize; ++i)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;

    stack.Push(temp);
    reference.push_back(temp);
  }

  uint64_t index, elements;
  for (uint64_t i = 0; i < testSize; i++)
  {
    index    = lfg() % testSize;
    elements = lfg() % testSize + 1;  // +1 is to ensure elements should always be >0
    uint64_t expected_elements = std::min(elements, (stack.size() - index));

    TestClass *objects = new TestClass[elements];
    stack.GetBulk(index, elements, objects);
    EXPECT_EQ(expected_elements, elements);
    for (uint64_t j = 0; j < elements; j++)
    {
      EXPECT_TRUE(reference[index + j] == objects[j])
          << " Values does not match in GetBulk at index " << j << " " << reference[j].value1
          << "    " << objects[j].value1 << std::endl;
    }

    free(objects);
  }
}
TEST(mmap_random_access_stack, set_bulk)
{
  constexpr uint64_t                        testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  MMapRandomAccessStack<TestClass>          stack;
  std::vector<TestClass>                    reference;

  stack.New("test_mmap.db");
  EXPECT_TRUE(stack.is_open());

  for (uint64_t i = 0; i < testSize; ++i)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;

    stack.Push(temp);
    reference.push_back(temp);
  }
  
  uint64_t index, elements, temp_size=0;

  //Setting bulk at the end of the stack. Size should be updated
  {
    elements           = lfg() % testSize;
    TestClass *objects = new TestClass[elements];
    stack.GetBulk(index, elements, objects);
    index = stack.size() ;
    temp_size = stack.size();

    stack.SetBulk(index, elements, objects);
    EXPECT_TRUE(temp_size + elements == stack.size());
    for (uint64_t j = 0; j < elements; j++)
    {
      TestClass obj;
      stack.Get(index + j, obj);
      EXPECT_TRUE(objects[j] == obj)
      << " Values does not match in SetBulk at index " << j << " " << obj.value1 << "    "
      << objects[j].value1 << std::endl;
    }
    delete []objects;
  }
    //Setting bulk at the edge of the stack where half elements would be overwritten 
    //so size should be incremented by half of elements
  {
    elements = lfg() % testSize;
    TestClass *objects = new TestClass[elements];
    stack.GetBulk(index, elements, objects);
    index = stack.size() - elements/2 ;
    temp_size = stack.size();

    stack.SetBulk(index, elements, objects);
    EXPECT_TRUE(temp_size + elements/2 == stack.size());
    for (uint64_t j = 0; j < elements; j++)
    {
      TestClass obj;
      stack.Get(index + j, obj);
      EXPECT_TRUE(objects[j] == obj)
      << " Values does not match in SetBulk at index " << j << " " << obj.value1 << "    "
      << objects[j].value1 << std::endl;
    }
    delete []objects;
  }
    
}
TEST(mmap_random_access_stack, file_writing_and_recovery)
{
  constexpr uint64_t                        testSize = 100;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  std::vector<TestClass>                    reference;

  {
    MMapRandomAccessStack<TestClass> stack;

    // Testing closures
    bool file_loaded  = false;
    bool file_flushed = false;

    stack.OnFileLoaded([&file_loaded] { file_loaded = true; });
    stack.OnBeforeFlush([&file_flushed] { file_flushed = true; });

    stack.New("test_mmap.db");

    EXPECT_TRUE(file_loaded == true);

    stack.SetExtraHeader(0x00deadbeefcafe00);
    EXPECT_TRUE(stack.header_extra() == 0x00deadbeefcafe00);

    // Fill with random numbers
    for (uint64_t i = 0; i < testSize; ++i)
    {
      uint64_t  random = lfg();
      TestClass temp;
      temp.value1 = random;
      temp.value2 = random & 0xFF;

      stack.Push(temp);
      reference.push_back(temp);
    }

    stack.Flush();
    EXPECT_TRUE(file_flushed == true);
  }

  // Create file if not exist while loading
  {
    std::string filename = "test_mmap_new.db";
    // delete if file already exist
    std::remove(filename.c_str());
    MMapRandomAccessStack<TestClass> stack;

    stack.Load("test_mmap_new.db", true);
    EXPECT_TRUE(stack.is_open());
    stack.Close();
  }

  // Check values against loaded file
  {
    MMapRandomAccessStack<TestClass> stack;

    stack.Load("test_mmap.db");
    EXPECT_TRUE(stack.header_extra() == 0x00deadbeefcafe00);

    {
      ASSERT_TRUE(stack.size() == reference.size());

      for (uint64_t i = 0; i < testSize; ++i)
      {
        TestClass temp;
        stack.Get(i, temp);
        ASSERT_TRUE(temp == reference[i]);
      }
    }
    stack.Close();
  }
}