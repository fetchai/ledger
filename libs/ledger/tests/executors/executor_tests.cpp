#include "core/make_unique.hpp"
#include "ledger/executor.hpp"

#include "fake_storage_unit.hpp"

#include <gtest/gtest.h>
#include <memory>

class ExecutorTests : public ::testing::Test {
protected:
  using underlying_executor_type = fetch::ledger::Executor;
  using executor_type = std::unique_ptr<underlying_executor_type>;
  using storage_type = std::shared_ptr<FakeStorageUnit>;

  void SetUp() override {
    storage_.reset(new FakeStorageUnit);
    executor_.reset(new underlying_executor_type(storage_));
  }

public:

  storage_type storage_;
  executor_type executor_;
};

TEST_F(ExecutorTests, Check) {



}
