#pragma once

enum class WorkloadProcessed {
  COMPLETE=0,
  NOT_COMPLETE=1,
  NOT_STARTED=2,
};

enum class WorkloadState {
  START,
  RESUME,
};

extern const char *const workloadProcessedNames[];
