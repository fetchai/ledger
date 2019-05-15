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

#include "core/sync/tickets.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

namespace fetch {
namespace core {

namespace {

class TicketsTest : public testing::Test
{
protected:
  using ThreadsContainer = std::vector<std::unique_ptr<std::thread>>;

  ThreadsContainer wait_with_no_timeout(Tickets &             ticket_under_test,
                                        Tickets::Count const &num_of_posts,
                                        Tickets::Count const &num_of_waits)
  {
    std::vector<std::unique_ptr<std::thread>> threads{num_of_waits};
    for (auto &t : threads)
    {
      t = std::make_unique<std::thread>([&ticket_under_test]() {
        std::this_thread::yield();
        ticket_under_test.Wait();
        std::this_thread::yield();
      });
    }

    for (Tickets::Count i = 0; i < num_of_posts; ++i)
    {
      std::this_thread::yield();
      ticket_under_test.Post();
      std::this_thread::yield();
    }

    return threads;
  }

  void wait_for_finalising_threads_for_wait_with_no_timeout(
      ThreadsContainer &threads, Tickets &ticket,
      Tickets::Count const &expected_number_of_threads_left_stuck_in_waiting_mode,
      bool const &          expected_number_of_threads_is_negative)
  {
    Tickets::Count           manually_posted{0};
    Tickets::Count           diff{0};
    Tickets::Count           prev_diff{std::numeric_limits<Tickets::Count>::max()};
    std::size_t              no_change_count{0};
    bool                     is_diff_positive      = true;
    bool                     prev_is_diff_positive = true;
    constexpr Tickets::Count no_change_count_threshold{4};

    // We expect, all threads from pool shall be finished in 8 secs for sure.
    std::this_thread::sleep_for(std::chrono::seconds{4});

    // Make sure all possibly still waiting threads are eliminated (= no stuck threads are left
    // over)
    while (no_change_count < no_change_count_threshold && manually_posted < 8)
    {
      std::this_thread::sleep_for(std::chrono::seconds{1});

      // Probing post (acquisition of the real `count`)
      Tickets::Count count{std::numeric_limits<Tickets::Count>::max()};
      ticket.Post(count);

      ++manually_posted;

      prev_diff        = diff;
      is_diff_positive = manually_posted >= count;
      diff             = is_diff_positive ? manually_posted - count : count - manually_posted;
      if (diff == prev_diff && is_diff_positive == prev_is_diff_positive)
      {
        ++no_change_count;
      }
      else
      {
        no_change_count = 0;
      }
      prev_is_diff_positive = is_diff_positive;
    }

    for (auto &t : threads)
    {
      t->join();
    }

    EXPECT_EQ(no_change_count_threshold, no_change_count);
    EXPECT_EQ(expected_number_of_threads_left_stuck_in_waiting_mode, diff);
    EXPECT_EQ(expected_number_of_threads_is_negative, is_diff_positive);
  }

  Tickets::Count test_wait_with_timeout(Tickets &             ticket_under_test,
                                        Tickets::Count const &num_of_posts,
                                        Tickets::Count const &num_of_waits)
  {
    std::atomic<Tickets::Count>               num_of_failed_waits{0};
    std::vector<std::unique_ptr<std::thread>> threads{num_of_waits};

    for (auto &t : threads)
    {
      t = std::make_unique<std::thread>([&ticket_under_test, &num_of_failed_waits]() {
        std::this_thread::yield();
        if (!ticket_under_test.Wait(std::chrono::seconds{8}))
        {
          std::this_thread::yield();
          num_of_failed_waits++;
        }
        std::this_thread::yield();
      });
    }

    for (Tickets::Count i = 0; i < num_of_posts; ++i)
    {
      std::this_thread::yield();
      ticket_under_test.Post();
      std::this_thread::yield();
    }

    for (auto &t : threads)
    {
      t->join();
    }

    return num_of_failed_waits;
  }
};

TEST_F(TicketsTest, basic_wait_post_cycle)
{
  Tickets ticket;

  std::thread x{[&ticket]() {
    std::this_thread::sleep_for(std::chrono::seconds{4});
    ticket.Post();
  }};
  EXPECT_TRUE(ticket.Wait(std::chrono::seconds{8}));
  x.join();
}

TEST_F(TicketsTest, basic_timeout)
{
  Tickets ticket;

  std::thread x{[]() { std::this_thread::sleep_for(std::chrono::seconds{8}); }};
  EXPECT_FALSE(ticket.Wait(std::chrono::seconds{4}));
  x.join();
}

TEST_F(TicketsTest, multiple_cycles)
{
  constexpr Tickets::Count num_of_posts{5};
  constexpr Tickets::Count num_of_waits{num_of_posts};
  // PRECONDITION for test OBJECTIVE
  ASSERT_EQ(num_of_waits, num_of_posts);

  Tickets    ticket;
  auto const num_of_failed_waits = test_wait_with_timeout(ticket, num_of_posts, num_of_waits);
  EXPECT_EQ(0, num_of_failed_waits);

  // Prove that there are **NO** posted tickets left over.
  EXPECT_FALSE(ticket.Wait(std::chrono::seconds{2}));
}

TEST_F(TicketsTest, multiple_cycles_fails_if_less_posts_than_waits)
{
  constexpr Tickets::Count num_of_posts{5};
  constexpr Tickets::Count num_of_waits{num_of_posts + 2};
  // PRECONDITION for test OBJECTIVE
  ASSERT_GT(num_of_waits, num_of_posts);

  Tickets    ticket;
  auto const num_of_failed_waits = test_wait_with_timeout(ticket, num_of_posts, num_of_waits);

  Tickets::Count const missing_posts = num_of_waits - num_of_posts;
  EXPECT_EQ(missing_posts, num_of_failed_waits);

  // Prove that there are **NO** posted tickets left over.
  EXPECT_FALSE(ticket.Wait(std::chrono::seconds{2}));
}

TEST_F(TicketsTest, multiple_cycles_fails_if_more_posts_than_waits)
{
  constexpr Tickets::Count num_of_waits{5};
  constexpr Tickets::Count num_of_posts{num_of_waits + 2};

  // PRECONDITION for test OBJECTIVE
  ASSERT_LT(num_of_waits, num_of_posts);
  Tickets    ticket;
  auto const num_of_failed_waits = test_wait_with_timeout(ticket, num_of_posts, num_of_waits);

  EXPECT_EQ(0, num_of_failed_waits);

  Tickets::Count const remaining_posts = num_of_posts - num_of_waits;

  for (Tickets::Count i = 0; i < remaining_posts; ++i)
  {
    // Exhaust all REMAINING  tickets.
    EXPECT_TRUE(ticket.Wait(std::chrono::seconds{2}));
  }

  // Prove that there are **NO** posted tickets left over.
  EXPECT_FALSE(ticket.Wait(std::chrono::seconds{2}));
}

TEST_F(TicketsTest, multiple_cycles_no_timeout)
{
  constexpr Tickets::Count num_of_waits{5};
  constexpr Tickets::Count num_of_posts{num_of_waits};

  // PRECONDITION for test OBJECTIVE
  ASSERT_EQ(num_of_waits, num_of_posts);

  Tickets          ticket;
  ThreadsContainer threads = wait_with_no_timeout(ticket, num_of_posts, num_of_waits);

  wait_for_finalising_threads_for_wait_with_no_timeout(threads, ticket, 0, true);
}

TEST_F(TicketsTest, multiple_cycles_no_timeout_fail_if_less_posts_than_waits)
{
  constexpr Tickets::Count num_of_posts{5};
  constexpr Tickets::Count num_of_waits{num_of_posts + 2};

  // PRECONDITION for test OBJECTIVE
  ASSERT_GT(num_of_waits, num_of_posts);

  Tickets          ticket;
  ThreadsContainer threads = wait_with_no_timeout(ticket, num_of_posts, num_of_waits);

  wait_for_finalising_threads_for_wait_with_no_timeout(threads, ticket, num_of_waits - num_of_posts,
                                                       num_of_waits >= num_of_posts);
}

TEST_F(TicketsTest, multiple_cycles_no_timeout_fail_if_more_posts_than_waits)
{
  constexpr Tickets::Count num_of_waits{5};
  constexpr Tickets::Count num_of_posts{num_of_waits + 2};

  // PRECONDITION for test OBJECTIVE
  ASSERT_LT(num_of_waits, num_of_posts);

  Tickets          ticket;
  ThreadsContainer threads = wait_with_no_timeout(ticket, num_of_posts, num_of_waits);

  wait_for_finalising_threads_for_wait_with_no_timeout(threads, ticket, num_of_posts - num_of_waits,
                                                       num_of_waits >= num_of_posts);
}

}  // namespace

}  // namespace core
}  // namespace fetch
