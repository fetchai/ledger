#pragma once

#include "BranchExecutorTask.hpp"

class DapManager;

std::shared_ptr<NodeExecutorTask> NodeExecutorFactory(
    const BranchExecutorTask::NodeDataType& data,
    std::shared_ptr<IdentifierSequence> input,
    std::shared_ptr<DapManager>& dap_manager
    );
