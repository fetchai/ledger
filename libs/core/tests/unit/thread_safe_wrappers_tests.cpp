//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/synchronisation/protected.hpp"
#include "core/synchronisation/waitable.hpp"

#include "gmock/gmock.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

using namespace fetch;
using namespace testing;

namespace {

template <typename Wrapper>
void recursively_increment_n_times(Wrapper &protected_value, uint8_t n)
{
  if (n > 0)
  {
    protected_value.ApplyVoid([&protected_value, n](auto &payload) {
      ++payload;
      recursively_increment_n_times(protected_value, static_cast<uint8_t>(n - 1));
    });
  }
}

template <typename Type>
bool is_read_only(Type & /*unused*/)
{
  return std::is_const<std::remove_reference_t<Type>>();
}

class MutexSpy
{
public:
  MOCK_METHOD0(lock, void());
  MOCK_METHOD0(unlock, void());
};

class PayloadSpy
{
public:
  MOCK_METHOD0(call, void());
};

class ThreadSafeWrapperTests : public Test
{
public:
  using Type      = int;
  using ConstType = Type const;

  static std::unique_ptr<MutexSpy> mutex_spy;

  ThreadSafeWrapperTests()
    : initial_value{5}
    , new_value{42}
  {
    mutex_spy = std::make_unique<MutexSpy>();
  }

  ~ThreadSafeWrapperTests() override
  {
    mutex_spy = nullptr;
  }

  class TestMutex
  {
  public:
    void lock()
    {
      assert(mutex_spy != nullptr);
      mutex_spy->lock();
    }

    void unlock()
    {
      assert(mutex_spy != nullptr);
      mutex_spy->unlock();
    }
  };

  template <template <typename, typename> class Wrapper>
  void wrapper_passes_ctor_arguments_to_its_payload()
  {
    Wrapper<std::vector<std::string>, std::mutex> protected_vector(3u, "abc");

    protected_vector.ApplyVoid([](auto &vector_payload) {
      EXPECT_EQ(vector_payload.size(), 3u);
      EXPECT_EQ(vector_payload[0], std::string{"abc"});
    });
  }

  template <template <typename, typename> class Wrapper>
  void const_protect_on_const_type_allows_read_only_access()
  {
    const Wrapper<ConstType, std::mutex> const_protected_const_value(initial_value);

    const_protected_const_value.ApplyVoid([this](auto &payload) {
      EXPECT_TRUE(is_read_only(payload));

      EXPECT_EQ(payload, initial_value);
    });
  }

  template <template <typename, typename> class Wrapper>
  void nonconst_protect_on_const_type_allows_read_only_access()
  {
    Wrapper<ConstType, std::mutex> protected_const_value(initial_value);

    protected_const_value.ApplyVoid([this](auto &payload) {
      EXPECT_TRUE(is_read_only(payload));

      EXPECT_EQ(payload, initial_value);
    });
  }

  template <template <typename, typename> class Wrapper>
  void const_protect_on_nonconst_type_allows_read_only_access()
  {
    const Wrapper<Type, std::mutex> const_protected_value(initial_value);

    const_protected_value.ApplyVoid([this](auto &payload) {
      EXPECT_TRUE(is_read_only(payload));

      EXPECT_EQ(payload, initial_value);
    });
  }

  template <template <typename, typename> class Wrapper>
  void nonconst_protect_on_nonconst_type_allows_read_and_write_access()
  {
    Wrapper<Type, std::mutex> protected_value(initial_value);

    protected_value.ApplyVoid([this](auto &payload) {
      EXPECT_FALSE(is_read_only(payload));

      EXPECT_EQ(payload, initial_value);
      payload = new_value;

      EXPECT_EQ(payload, new_value);
    });

    protected_value.ApplyVoid([this](auto &payload) { EXPECT_EQ(payload, new_value); });
  }

  template <template <typename, typename> class Wrapper>
  void handler_return_value_is_passed_to_Apply()
  {
    Wrapper<Type, std::mutex> protected_value(initial_value);

    auto const result = protected_value.Apply([](auto &payload) -> std::vector<Type> {
      return {payload, 3 * payload};
    });

    auto const expected = std::vector<Type>{initial_value, 3 * initial_value};

    EXPECT_EQ(result, expected);
  }

  template <template <typename, typename> class Wrapper>
  void wrapper_may_be_used_with_arbitrary_mutex_type()
  {
    auto const iterations = 5;

    // Use recursive mutex
    Wrapper<Type, std::recursive_mutex> protected_value_with_recursive_mutex{
        static_cast<uint8_t>(initial_value)};

    // would deadlock with non-recursive mutex
    recursively_increment_n_times(protected_value_with_recursive_mutex, iterations);

    protected_value_with_recursive_mutex.ApplyVoid([this](auto &payload) {
      auto const final_value = initial_value + iterations;
      EXPECT_EQ(payload, final_value);
    });
  }

  template <template <typename, typename> class Wrapper>
  void call_to_Apply_locks_and_then_releases_the_mutex()
  {
    PayloadSpy payload_spy;

    InSequence in_sequence;
    EXPECT_CALL(*mutex_spy, lock());
    EXPECT_CALL(payload_spy, call());
    EXPECT_CALL(*mutex_spy, unlock());

    Wrapper<Type, TestMutex> protected_value_with_test_mutex{};

    protected_value_with_test_mutex.ApplyVoid([&payload_spy](auto &) { payload_spy.call(); });
  }

  template <template <typename, typename> class Wrapper>
  void each_call_to_Apply_locks_mutex_independently()
  {
    PayloadSpy payload_spy;

    InSequence in_sequence;
    EXPECT_CALL(*mutex_spy, lock());
    EXPECT_CALL(payload_spy, call());
    EXPECT_CALL(*mutex_spy, lock());
    EXPECT_CALL(payload_spy, call());
    EXPECT_CALL(*mutex_spy, unlock()).Times(2);

    Wrapper<Type, TestMutex> protected_value_with_test_mutex{};
    protected_value_with_test_mutex.ApplyVoid(
        [&protected_value_with_test_mutex, &payload_spy](auto &) {
          payload_spy.call();
          protected_value_with_test_mutex.ApplyVoid([&payload_spy](auto &) { payload_spy.call(); });
        });
  }

  ConstType initial_value;
  ConstType new_value;
};

std::unique_ptr<MutexSpy> ThreadSafeWrapperTests::mutex_spy = nullptr;

TEST_F(ThreadSafeWrapperTests, wrapper_passes_ctor_arguments_to_its_payload)
{
  wrapper_passes_ctor_arguments_to_its_payload<Protected>();
  wrapper_passes_ctor_arguments_to_its_payload<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, const_protect_on_const_type_allows_read_only_access)
{
  const_protect_on_const_type_allows_read_only_access<Protected>();
  const_protect_on_const_type_allows_read_only_access<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, nonconst_protect_on_const_type_allows_read_only_access)
{
  nonconst_protect_on_const_type_allows_read_only_access<Protected>();
  nonconst_protect_on_const_type_allows_read_only_access<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, const_protect_on_nonconst_type_allows_read_only_access)
{
  const_protect_on_nonconst_type_allows_read_only_access<Protected>();
  const_protect_on_nonconst_type_allows_read_only_access<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, nonconst_protect_on_nonconst_type_allows_read_and_write_access)
{
  nonconst_protect_on_nonconst_type_allows_read_and_write_access<Protected>();
  nonconst_protect_on_nonconst_type_allows_read_and_write_access<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, handler_return_value_is_passed_to_Apply)
{
  handler_return_value_is_passed_to_Apply<Protected>();
  handler_return_value_is_passed_to_Apply<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, wrapper_may_be_used_with_arbitrary_mutex_type)
{
  wrapper_may_be_used_with_arbitrary_mutex_type<Protected>();
  wrapper_may_be_used_with_arbitrary_mutex_type<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, call_to_Apply_locks_and_then_releases_the_mutex)
{
  call_to_Apply_locks_and_then_releases_the_mutex<Protected>();
  call_to_Apply_locks_and_then_releases_the_mutex<Waitable>();
}

TEST_F(ThreadSafeWrapperTests, each_call_to_Apply_locks_mutex_independently)
{
  each_call_to_Apply_locks_mutex_independently<Protected>();
  each_call_to_Apply_locks_mutex_independently<Waitable>();
}

}  // namespace
