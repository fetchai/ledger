#include "core/make_unique.hpp"
#include "ledger/executor.hpp"

#include <gtest/gtest.h>
#include <memory>

class ExecutorTests : public ::testing::Test {
protected:
  using underlying_executor_type = fetch::ledger::Executor;
  using executor_type = std::unique_ptr<underlying_executor_type>;

  void SetUp() override {
    executor_.reset(new underlying_executor_type);
  }

public:

  executor_type executor_;
};

TEST_F(ExecutorTests, Check) {
}
