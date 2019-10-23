#pragma once

#include "network/fetch_asio.hpp"
#include <list>
#include <vector>

namespace google {
namespace protobuf {
class Message;
};
};  // namespace google

template <typename TXType>
class IMessageWriter
{
public:
  using mutable_buffer       = asio::mutable_buffer;
  using mutable_buffers      = std::vector<mutable_buffer>;
  using consumed_needed_pair = std::pair<std::size_t, std::size_t>;
  using TXQ                  = std::list<TXType>;

  IMessageWriter()
  {}
  virtual ~IMessageWriter()
  {}

  virtual consumed_needed_pair initial()
  {
    return consumed_needed_pair(0, 0);
  }

  virtual consumed_needed_pair CheckForSpace(const mutable_buffers &space, TXQ &txq) = 0;

protected:
private:
  IMessageWriter(const IMessageWriter &other) = delete;
  IMessageWriter &operator=(const IMessageWriter &other)  = delete;
  bool            operator==(const IMessageWriter &other) = delete;
  bool            operator<(const IMessageWriter &other)  = delete;
};

// namespace std { template<> void swap(IMessageWriter& lhs, IMessageWriter& rhs) { lhs.swap(rhs); }
// } std::ostream& operator<<(std::ostream& os, const IMessageWriter &output) {}
