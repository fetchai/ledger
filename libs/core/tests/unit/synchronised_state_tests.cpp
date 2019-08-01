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

#include "core/threading/synchronised_state.hpp"

#include "gmock/gmock.h"

#include <cstdint>
#include <thread>

namespace {

using namespace fetch;
using namespace testing;

class SynchronisedStateTests : public Test
{
public:
  SynchronisedState<uint32_t> waitable{0u};
};

TEST_F(SynchronisedStateTests, WaitFor_returns_when_the_condition_is_true)
{
  auto check = [this]() -> void {
    for (auto i = 0; i < 10000; ++i)
    {
      waitable.Wait([](auto const &number) -> bool { return number > 5000; });
      waitable.Apply([](auto const &number) -> void { ASSERT_THAT(number, Gt(5000)); });
    }
  };

  auto increment = [this]() -> void {
    for (auto i = 0; i < 10000; ++i)
    {
      waitable.Apply([](auto &number) -> void { ++number; });
    }
  };

  auto check_thread     = std::thread(std::move(check));
  auto increment_thread = std::thread(std::move(increment));

  check_thread.join();
  increment_thread.join();
}

}  // namespace

//???
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

class xxxProtectTests : public Test
{
public:
  using Type      = int;
  using ConstType = Type const;

  static std::unique_ptr<MutexSpy> mutex_spy;

  xxxProtectTests()
    : initial_value{5}
    , new_value{42}
    , const_protected_const_value(Type{initial_value})
    , protected_const_value(Type{initial_value})
    , const_protected_value(Type{initial_value})
    , protected_value(Type{initial_value})
  {
    mutex_spy = std::make_unique<MutexSpy>();
  }

  ~xxxProtectTests() override
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

  SynchronisedState<ConstType> const const_protected_const_value;
  SynchronisedState<ConstType>       protected_const_value;
  SynchronisedState<Type> const      const_protected_value;
  SynchronisedState<Type>            protected_value;
};

std::unique_ptr<MutexSpy> xxxProtectTests::mutex_spy = nullptr;

TEST_F(xxxProtectTests, protect_passes_ctor_arguments_to_its_payload)
{
  Protect<std::vector<std::string>> protected_vector(3u, "abc");

  protected_vector.Apply([](auto &vector_payload) -> void {
    EXPECT_EQ(vector_payload.size(), 3u);
    EXPECT_EQ(vector_payload[0], std::string{"abc"});
  });
}

TEST_F(xxxProtectTests, const_protect_on_const_type_allows_read_only_access)
{
  const_protected_const_value.Apply([this](auto &payload) -> void {
    EXPECT_TRUE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
  });
}

TEST_F(xxxProtectTests, nonconst_protect_on_const_type_allows_read_only_access)
{
  protected_const_value.Apply([this](auto &payload) -> void {
    EXPECT_TRUE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
  });
}

TEST_F(xxxProtectTests, const_protect_on_nonconst_type_allows_read_only_access)
{
  const_protected_value.Apply([this](auto &payload) -> void {
    EXPECT_TRUE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
  });
}

TEST_F(xxxProtectTests, nonconst_protect_on_nonconst_type_allows_read_and_write_access)
{
  protected_value.Apply([this](auto &payload) -> void {
    EXPECT_FALSE(is_read_only(payload));

    EXPECT_EQ(payload, initial_value);
    payload = new_value;

    EXPECT_EQ(payload, new_value);
  });

  protected_value.Apply([this](auto &payload) -> void { EXPECT_EQ(payload, new_value); });
}

TEST_F(xxxProtectTests, handler_return_value_is_passed_to_Apply)
{
  auto const result = protected_value.Apply([](auto &payload) -> std::vector<int> {
    return {payload, 3 * payload};
  });

  auto const expected = std::vector<int>{initial_value, 3 * initial_value};

  EXPECT_EQ(result, expected);
}

TEST_F(xxxProtectTests, protect_may_be_used_with_arbitrary_mutex_type)
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

TEST_F(xxxProtectTests, call_to_Apply_locks_the_mutex)
{
  InSequence in_sequence;
  EXPECT_CALL(*mutex_spy, lock());
  EXPECT_CALL(*mutex_spy, unlock());

  Protect<Type, TestMutex> protected_value_with_test_mutex{};
  protected_value_with_test_mutex.Apply([](auto &) -> void {});
}

TEST_F(xxxProtectTests, each_call_to_Apply_locks_mutex_independently)
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
