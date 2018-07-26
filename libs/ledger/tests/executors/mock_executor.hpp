#pragma once

#include "ledger/executor_interface.hpp"
#include "fake_executor.hpp"

#include <gmock/gmock.h>

class MockExecutor : public fetch::ledger::ExecutorInterface {
public:

  MockExecutor() {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, Execute(_,_,_))
      .WillByDefault(Invoke(&fake_, &FakeExecutor::Execute));
  }

  MOCK_METHOD3(Execute, Status(tx_digest_type const &, std::size_t, lane_set_type const &));

private:

  FakeExecutor fake_;
};

