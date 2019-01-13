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

#include "core/containers/queue.hpp"

#include <atomic>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

template <class Stream, class Array>
Stream &printArray(Stream &cerr, Array const &a)
{
  static constexpr std::size_t width = 80;
  if (!a.empty())
  {
    auto        rle{a.front()};
    std::string buf{std::to_string(rle)};
    std::size_t count{1};

    for (std::size_t i{1}; i < a.size() - 1; ++i)
    {
      auto val{a[i]};
      if (val != rle)
      {
        if (count > 1)
        {
          buf += " x " + std::to_string(count);
          count = 1;
        }
        buf += ',';
        if (buf.size() >= width)
        {
          cerr << buf << '\n';
          buf = std::to_string(val);
        }
        else
        {
          buf += ' ' + std::to_string(val);
        }
        rle = val;
      }
      else
      {
        ++count;
      }
    }
    if (a.size() > 1)
    {
      auto val{a.back()};
      if (val != rle)
      {
        if (count > 1)
        {
          buf += " x " + std::to_string(count);
          count = 1;
        }
        buf += ", " + std::to_string(val);
      }
      else
      {
        buf += " x " + std::to_string(count + 1);
      }
    }
    cerr << buf << '\n';
  }
  return cerr;
}

class QueueTests : public ::testing::Test
{
protected:
  static constexpr std::size_t ELEMENT_SIZE = 1024;
  using Element                             = std::array<uint16_t, ELEMENT_SIZE>;

  template <std::size_t NUM_THREADS, typename Queue>
  void RunTestMultiProducerTest(Queue &queue)
  {
    // ensure the queue element type is valid
    static_assert(std::is_same<typename Queue::Element, Element>::value, "");

    using ThreadPtr  = std::unique_ptr<std::thread>;
    using ThreadList = std::vector<ThreadPtr>;

    static constexpr std::size_t NUM_LOOPS = 50;
    static constexpr std::size_t NUM_ELEMENTS_PER_THREAD =
        (Queue::QUEUE_LENGTH * NUM_LOOPS) / NUM_THREADS;
    static constexpr std::size_t TOTAL_NUM_ELEMENTS = NUM_THREADS * NUM_ELEMENTS_PER_THREAD;

    ThreadList threads(NUM_THREADS);

    uint16_t thread_idx{0};
    for (auto &thread : threads)
    {
      thread = std::make_unique<std::thread>([&queue, thread_idx]() {
        // create the boilerplate element
        Element element;
        std::fill(element.begin(), element.end(), thread_idx);

        for (std::size_t i = 0; i < NUM_ELEMENTS_PER_THREAD; ++i)
        {
          // uniquely identify the element
          element.front() = static_cast<uint16_t>(i);
          element.back()  = static_cast<uint16_t>(i);

          // add the element to the queue
          queue.Push(element);
        }
      });

      ++thread_idx;
    }

    // create the counters
    std::array<std::size_t, NUM_THREADS> counters{};

    // check initialisation
    for (auto const &count : counters)
    {
      ASSERT_EQ(count, 0);
    }

    Element element;
    for (std::size_t i = 0; i < TOTAL_NUM_ELEMENTS; ++i)
    {
      // pop the element from the queue
      ASSERT_TRUE(queue.Pop(element, std::chrono::seconds{4}));

      if (element.front() != element.back())
      {
        printArray(std::cerr << "Corrupt array:\n", element) << '\n';
      }

      // thread the thread index counter
      ASSERT_EQ(element.front(), element.back());

      // check the contents of the array
      uint16_t const thread_id = element[1];
      for (std::size_t j = 2; j < (ELEMENT_SIZE - 1); ++j)
      {
        if (thread_id != element[j])
        {
          printArray(std::cerr << "Corrupt array:\n", element) << '\n';
        }
        ASSERT_EQ(thread_id, element[j]);
      }

      // update the thread counters
      counters.at(thread_id) += 1;
    }

    // ensure all the counters are correct
    for (auto const &count : counters)
    {
      ASSERT_EQ(count, NUM_ELEMENTS_PER_THREAD);
    }

    // wait for all the threads to complete
    for (auto &thread : threads)
    {
      thread->join();
    }
    threads.clear();
  }
};

TEST_F(QueueTests, CheckMultiProducerSingleConsumer)
{
  static constexpr std::size_t QUEUE_SIZE  = 1024;
  static constexpr std::size_t NUM_THREADS = 8;

  fetch::core::MPSCQueue<Element, QUEUE_SIZE> queue;
  RunTestMultiProducerTest<NUM_THREADS>(queue);
}

TEST_F(QueueTests, CheckMultiProducerMultiConsumer)
{
  static constexpr std::size_t QUEUE_SIZE  = 1024;
  static constexpr std::size_t NUM_THREADS = 8;

  fetch::core::MPMCQueue<Element, QUEUE_SIZE> queue;
  RunTestMultiProducerTest<NUM_THREADS>(queue);
}

}  // namespace
