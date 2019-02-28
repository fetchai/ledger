//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "vm_modules/vm_factory.hpp"
#include "vm/state_sentinel.hpp"

#include <gtest/gtest.h>

using namespace fetch;
using namespace fetch::vm_modules;

// Wrapper class for strings/memory to avoid needing byte array from core code
class ByteWrapper
{
public:
  explicit ByteWrapper()
  {}

  // copy data into bytewrapper
  ByteWrapper(uint8_t const *const data, uint64_t data_size)
  {
    auto bytes = (uint8_t *)malloc(data_size);
    memset(bytes, 0, data_size);
    memcpy(bytes, data, data_size);
    length_ = data_size;
    data_   = bytes;
  }

  // create new bytewrapper
  explicit ByteWrapper(uint64_t data_size)
  {
    uint8_t *bytes = (uint8_t *)malloc(data_size);
    memset(bytes, 0, data_size);
    length_ = data_size;
    data_   = bytes;
  }

  explicit ByteWrapper(ByteWrapper &&rhs)
  {
    data_       = rhs.data_;
    length_     = rhs.length_;
    rhs.length_ = 0;
  }

  ByteWrapper &operator=(ByteWrapper &&rhs)
  {
    data_       = rhs.data_;
    length_     = rhs.length_;
    rhs.length_ = 0;
    return *this;
  }

  ~ByteWrapper()
  {
    if (length_ > 0)
    {
      free(data_);
    }
  }

  bool operator<(ByteWrapper const &rhs) const
  {
    if (length_ != rhs.length_)
    {
      return length_ < rhs.length_;
    }

    return memcmp(data_, rhs.data_, length_) > 0;
  }

  uint8_t *data()
  {
    return data_;
  }

  void set_data(uint8_t *data)
  {
    data_ = data;
  }

private:
  uint8_t *data_   = nullptr;
  uint64_t length_ = 0;
};

class DummyReadWriteInterface : public vm::ReadWriteInterface
{
public:
  ~DummyReadWriteInterface() = default;

  bool exists(uint8_t const *const key, uint64_t key_size, bool &exists) override
  {
    ByteWrapper key_wrapper{key, key_size};
    exists = false;

    if (dummy_db_.find(key_wrapper) != dummy_db_.end())
    {
      exists = true;
    }

    return true;
  }

  bool read(uint8_t *dest, uint64_t dest_size, uint8_t const *const key, uint64_t key_size) override
  {

    ByteWrapper key_wrapper{key, key_size};

    if (dummy_db_.find(key_wrapper) == dummy_db_.end())
    {
      // Create memory
      ByteWrapper mem{dest_size};

      // Copy to caller
      memcpy(dest, mem.data(), dest_size);

      // Save for later
      dummy_db_[std::move(key_wrapper)] = std::move(mem);
    }
    else
    {
      uint8_t *data_ptr = dummy_db_[std::move(key_wrapper)].data();
      memcpy(dest, data_ptr, dest_size);
    }

    return true;
  }

  bool write(uint8_t const *const source, uint64_t dest_size, uint8_t const *const key,
             uint64_t key_size) override
  {
    ByteWrapper key_wrapper{key, key_size};

    // Create new memory if neccessary
    if (dummy_db_.find(key_wrapper) == dummy_db_.end())
    {
      // Create memory
      ByteWrapper mem{dest_size};

      // Copy to caller
      memcpy(mem.data(), source, dest_size);

      // Save for later
      dummy_db_[std::move(key_wrapper)] = std::move(mem);
    }
    else
    {
      auto &byte_wrapper = dummy_db_.at(key_wrapper);

      uint8_t *data_ptr = byte_wrapper.data();

      memcpy(data_ptr, source, dest_size);
    }

    return true;
  }

  template <typename T>
  T Lookup(std::string const &str)
  {
    T ret;
    if (!read(reinterpret_cast<uint8_t *>(&ret), sizeof(T),
              reinterpret_cast<uint8_t const *const>(str.c_str()), str.size()))
    {
      throw std::runtime_error("Failed to lookup data value!");
    }
    return ret;
  }

private:
  std::map<ByteWrapper, ByteWrapper> dummy_db_;
};

class VMTests : public ::testing::Test
{
protected:
  using Module = std::shared_ptr<fetch::vm::Module>;
  using VM     = std::unique_ptr<fetch::vm::VM>;

  VMTests()
  {}

  void SetUp() override
  {}

  template <typename T>
  void AddBinding(std::string const &name, T function)
  {
    module_->CreateFreeFunction(name, function);
  }

  bool Compile(std::string const &source)
  {
    std::vector<std::string> errors = VMFactory::Compile(module_, source, script_);

    for (auto const &error : errors)
    {
      std::cerr << error << std::endl;
    }

    return errors.size() == 0;
  }

  bool Execute(std::string const &function = "main")
  {
    vm_ = VMFactory::GetVM(module_);
    std::string              error;
    std::vector<std::string> print_strings;
    fetch::vm::Variant       output;

    // Attach our state
    vm_->SetIOInterface(&interface_);

    // Execute our fn
    if (!vm_->Execute(script_, function, error, print_strings, output))
    {
      std::cerr << "Runtime error: " << error << std::endl;
      return false;
    }

    for (auto const &i : print_strings)
    {
      std::cerr << "output: " << i << std::endl;
    }

    return true;
  }

  DummyReadWriteInterface &GetState()
  {
    return interface_;
  }

  Module                  module_ = VMFactory::GetModule();
  VM                      vm_;
  fetch::vm::Script       script_;
  DummyReadWriteInterface interface_;
};

// Test we can compile and run a fairly inoffensive smart contract
TEST_F(VMTests, CheckCompileAndExecute)
{
  const std::string source =
      " function main() "
      "   Print(\"Hello, world\");"
      " endfunction ";

  bool res = Compile(source);

  EXPECT_EQ(res, true);

  res = Execute();

  EXPECT_EQ(res, true);
}

TEST_F(VMTests, CheckCompileAndExecuteAltStrings)
{
  const std::string source =
      " function main() "
      "   Print('Hello, world');"
      " endfunction ";

  bool res = Compile(source);

  EXPECT_EQ(res, true);

  res = Execute();

  EXPECT_EQ(res, true);
}

// Test to add a custom binding that will increment this counter when
// the smart contract is executed
static int32_t binding_called_count = 0;

static void CustomBinding(fetch::vm::VM * /*vm*/)
{
  binding_called_count++;
}

TEST_F(VMTests, CheckCustomBinding)
{
  const std::string source =
      " function main() "
      "   CustomBinding();"
      " endfunction ";

  EXPECT_EQ(binding_called_count, 0);

  AddBinding("CustomBinding", &CustomBinding);

  bool res = Compile(source);

  EXPECT_EQ(res, true);

  for (uint64_t i = 0; i < 3; ++i)
  {
    res = Execute();
    EXPECT_EQ(res, true);
  }

  EXPECT_EQ(binding_called_count, 3);
}

TEST_F(VMTests, CheckCustomBindingWithState)
{
  const std::string source =
      "function main()                                      \n "
      " var s = State<Int32>('hello');                      \n "
      " Print('The STATE result is: ' + toString(s.get())); \n "
      " s.set(8);                                           \n "
      "                                                     \n "
      " endfunction                                         \n ";

  bool res = Compile(source);

  for (uint64_t i = 0; i < 3; ++i)
  {
    res = Execute();
    EXPECT_EQ(res, true);
  }

  int32_t out = GetState().Lookup<int32_t>("hello");

  EXPECT_EQ(out, 8);
}

TEST_F(VMTests, CheckCustomBindingWithStateDefault)
{
  const std::string source =
      "function main()                                      \n "
      " var s = State<Int32>('hello', 9);                   \n "
      " if(s.existed())                                     \n "
      "   Print('Recovered from file');                     \n "
      " else                                                \n "
      "   Print('Not recovered from file');                 \n "
      " endif                                               \n "
      " Print('The STATE result is: ' + toString(s.get())); \n "
      " s.set(8);                                           \n "
      "                                                     \n "
      " endfunction                                         \n ";

  bool res = Compile(source);

  for (uint64_t i = 0; i < 3; ++i)
  {
    res = Execute();
    EXPECT_EQ(res, true);
  }

  int32_t out = GetState().Lookup<int32_t>("hello");

  EXPECT_EQ(out, 8);
}
