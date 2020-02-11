#pragma once
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

#include "network/fetch_asio.hpp"
#include <functional>
#include <iostream>
#include <mutex>
#include <vector>

class RingBuffer
{
public:
  using Mutex          = std::mutex;
  using Lock           = std::lock_guard<Mutex>;
  using buffer         = asio::const_buffer;
  using mutable_buffer = asio::mutable_buffer;

  using byte = std::uint8_t;

  using SignalReady = std::function<void()>;

protected:
  void ignored()
  {}

public:
  RingBuffer(const RingBuffer &other) = delete;
  RingBuffer &operator=(const RingBuffer &other)  = delete;
  bool        operator==(const RingBuffer &other) = delete;
  bool        operator<(const RingBuffer &other)  = delete;

  explicit RingBuffer(size_t size)
  {
    this->size = size;
    store      = static_cast<byte *>(malloc(size));
    clear();
  }

  virtual ~RingBuffer()
  {
    free(store);
  }

  void clear()
  {
    this->freeSpace = this->size;
    this->writep    = 0;
    this->readp     = 0;
  }

  bool empty() const
  {
    return GetFreeSpace() == size;
  }

  mutable_buffer GetSpaceBuffer()
  {
    Lock lock(mut);
    if (GetFreeSpace() == 0)
    {
      return {nullptr, 0};
    }
    return mutable_buffer(AddressOf(writep % size),
                          std::min(writep + LocklessGetFreeSpace(), size) - writep);
  }

  buffer GetDataBuffer()
  {
    Lock lock(mut);
    if (GetFreeSpace() == size)
    {
      return {nullptr, 0};
    }
    return buffer(AddressOf(readp % size),
                  std::min(readp + LocklessGetDataAvailable(), size) - readp);
  }

  std::vector<mutable_buffer> GetSpaceBuffers()
  {
    Lock                        lock(mut);
    std::vector<mutable_buffer> r;
    if (LocklessGetFreeSpace() > 0)
    {
      size_t buffer1size = std::min(writep + LocklessGetFreeSpace(), size) - writep;
      size_t buffer2size = LocklessGetFreeSpace() - buffer1size;
      r.emplace_back(AddressOf(writep), buffer1size);
      if (buffer2size != 0u)
      {
        r.emplace_back(AddressOf(0), buffer2size);
      }
    }
    return r;
  }

  std::vector<buffer> GetDataBuffers()
  {
    Lock                lock(mut);
    std::vector<buffer> r;
    if (LocklessGetDataAvailable() < size)
    {
      size_t buffer1size = std::min(readp + LocklessGetDataAvailable(), size) - readp;
      size_t buffer2size = LocklessGetDataAvailable() - buffer1size;
      r.emplace_back(AddressOf(readp), buffer1size);
      if (buffer2size != 0u)
      {
        r.emplace_back(AddressOf(0), buffer2size);
      }
    }
    return r;
  }

  void MarkSpaceUsed(size_t amount)
  {
    std::size_t prevAvail = 0;
    {
      Lock lock(mut);
      writep += amount;
      writep %= size;
      prevAvail = LocklessGetDataAvailable();
      freeSpace -= amount;
    }
    if (prevAvail == 0u)
    {
      signalDataReady();
    }
  }

  void MarkDataUsed(size_t amount)
  {
    std::size_t prevSpace = 0;
    {
      Lock lock(mut);
      readp += amount;
      readp %= size;
      prevSpace = LocklessGetFreeSpace();
      freeSpace += amount;
    }
    if (prevSpace == 0u)
    {
      signalSpaceReady();
    }
  }

  const void *AddressOf(size_t index) const
  {
    return store + index;
  }
  void *AddressOf(size_t index)
  {
    return store + index;
  }

  size_t GetFreeSpace() const
  {
    Lock lock(mut);
    return LocklessGetFreeSpace();
  }
  size_t GetDataAvailable() const
  {
    Lock lock(mut);
    return LocklessGetDataAvailable();
  }

  bool HasFreeSpace() const
  {
    Lock lock(mut);
    return LocklessGetFreeSpace() > 0;
  }
  bool HasDataAvailable() const
  {
    Lock lock(mut);
    return LocklessGetDataAvailable() > 0;
  }

protected:
  size_t        size;
  size_t        freeSpace;
  size_t        readp, writep;
  byte *        store;
  mutable Mutex mut;

  SignalReady signalSpaceReady = []() {};
  SignalReady signalDataReady  = []() {};

  size_t LocklessGetFreeSpace() const
  {
    return freeSpace;
  }
  size_t LocklessGetDataAvailable() const
  {
    return size - freeSpace;
  }
};
