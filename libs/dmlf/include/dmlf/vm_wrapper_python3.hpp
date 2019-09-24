#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class vm_wrapper_python3
{
public:
  using OutputHandler = VmWrapperInterface::OutputHandler;
  using InputHandler  = VmWrapperInterface::InputHandler;
  using Params        = VmWrapperInterface::Params;
  using Flags         = VmWrapperInterface::Flags;
  using Status        = VmWrapperInterface::Status;

  vm_wrapper_python3()
  {
  }
  virtual ~vm_wrapper_python3()
  {
  }
  virtual void SetStdout(OutputHandler oh)   { oh+ = oh; }
  virtual void SetStdin(InputHandler ih)     { oi+ = ih; }
  virtual void SetStderr(OutputHandler eh)   { oe+ = eh; }
protected:
  OutputHandler oh_;
  Status status_ = VmWrapperInterface::WAITING;
  OutputHandler eh_;
  InputHandler ih_;
private:
  vm_wrapper_python3(const vm_wrapper_python3 &other) = delete;
  vm_wrapper_python3 &operator=(const vm_wrapper_python3 &other) = delete;
  bool operator==(const vm_wrapper_python3 &other) = delete;
  bool operator<(const vm_wrapper_python3 &other) = delete;
};
