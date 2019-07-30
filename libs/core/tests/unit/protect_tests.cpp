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

#include "core/threading/protect.hpp"

#include "gmock/gmock.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using namespace fetch;
using namespace testing;

template <typename T, typename M>
void recursively_increment_n_times(typename fetch::Protect<T, M> &protected_value, uint8_t n)
{
  if (n > 0)
  {
    protected_value.Apply([&protected_value, n](auto &payload) -> void {
      ++payload;
      recursively_increment_n_times(protected_value, static_cast<uint8_t>(n - 1));
    });
  }
}

template <typename Type>
bool is_read_only(Type &)
{
  return std::is_const<std::remove_reference_t<Type>>();
}

class MutexSpy
{
public:
  MOCK_METHOD0(lock, void());
  MOCK_METHOD0(unlock, void());
};

class ProtectTests : public Test
{
public:
  using Type      = int;
  using ConstType = Type const;

  static std::unique_ptr<MutexSpy> mutex_spy;

  ProtectTests()
    : initial_value{5}
    , new_value{42}
    , const_protected_const_value(Type{initial_value})
    , protected_const_value(Type{initial_value})
    , const_protected_value(Type{initial_value})
    , protected_value(Type{initial_value})
  {
    mutex_spy = std::make_unique<MutexSpy>();
  }

  ~ProtectTests() override
  {
    mutex_spy.reset();
  }

  class TestMutex
  {
  public:
    void lock()
    {
      mutex_spy->lock();
    }

    void unlock()
    {
      mutex_spy->unlock();
    }
  };

  ConstType initial_value;
  ConstType new_value;

  Protect<ConstType> const const_protected_const_value;
  Protect<ConstType>       protected_const_value;
  Protect<Type> const      const_protected_value;
  Protect<Type>            protected_value;
};

std::unique_ptr<MutexSpy> ProtectTests::mutex_spy = nullptr;

TEST_F(ProtectTests, protect_passes_ctor_arguments_to_its_payload)
{
  Protect<std::vector<std::string>> protected_vector(3u, "abc");

  protected_vector.Apply([](auto &vector_payload) -> void {
    EXPECT_EQ(vector_payload.size(), 3u);
    EXPECT_EQ(vector_payload[0], std::string{"abc"});
  });
}

TEST_F(ProtectTests, const_protect_on_const_type_allows_read_only_access)
{
  const_protected_const_value.Apply([this](auto &payload) -> void {
    EXPECT_TRUE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
  });
}

TEST_F(ProtectTests, nonconst_protect_on_const_type_allows_read_only_access)
{
  protected_const_value.Apply([this](auto &payload) -> void {
    EXPECT_TRUE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
  });
}

TEST_F(ProtectTests, const_protect_on_nonconst_type_allows_read_only_access)
{
  const_protected_value.Apply([this](auto &payload) -> void {
    EXPECT_TRUE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
  });
}

TEST_F(ProtectTests, nonconst_protect_on_nonconst_type_allows_read_and_write_access)
{
  protected_value.Apply([this](auto &payload) -> void {
    EXPECT_FALSE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
    payload = new_value;

    EXPECT_EQ(payload, new_value);
  });

  protected_value.Apply([this](auto &payload) -> void { EXPECT_EQ(payload, new_value); });
}

TEST_F(ProtectTests, handler_return_value_is_passed_to_Apply)
{
  auto const result = protected_value.Apply([](auto &payload) -> std::vector<int> {
    return {payload, 3 * payload};
  });

  auto const expected = std::vector<int>{initial_value, 3 * initial_value};

  EXPECT_EQ(result, expected);
}

TEST_F(ProtectTests, protect_may_be_used_with_arbitrary_mutex_type)
{
  auto const initial_value = 123;
  auto const iterations    = 5;

  Protect<Type, std::recursive_mutex> protected_value_with_recursive_mutex{
      static_cast<uint8_t>(initial_value)};

  // would deadlock with non-recursive mutex
  recursively_increment_n_times(protected_value_with_recursive_mutex, iterations);

  protected_value_with_recursive_mutex.Apply([](auto &payload) -> void {
    auto const final_value = initial_value + iterations;
    EXPECT_EQ(payload, final_value);
  });
}

TEST_F(ProtectTests, call_to_Apply_locks_the_mutex)
{
  InSequence in_sequence;
  EXPECT_CALL(*mutex_spy, lock());
  EXPECT_CALL(*mutex_spy, unlock());

  Protect<Type, TestMutex> protected_value_with_test_mutex{};
  protected_value_with_test_mutex.Apply([](auto &) -> void {});
}

TEST_F(ProtectTests, each_call_to_Apply_locks_mutex_independently)
{
  InSequence in_sequence;
  EXPECT_CALL(*mutex_spy, lock()).Times(2);
  EXPECT_CALL(*mutex_spy, unlock()).Times(2);

  Protect<Type, TestMutex> protected_value_with_test_mutex{};
  protected_value_with_test_mutex.Apply([&protected_value_with_test_mutex](auto &) -> void {
    protected_value_with_test_mutex.Apply([](auto &) -> void {});
  });
}

}  // namespace
