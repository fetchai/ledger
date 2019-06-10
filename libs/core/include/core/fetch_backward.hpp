#pragma once
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

#include "core/logger.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-Wsign-compare"
#endif

#include <backward.hpp>
#include <signal.h>

namespace fetch {
namespace log {

#if defined(BACKWARD_SYSTEM_LINUX) || defined(BACKWARD_SYSTEM_DARWIN)

class FetchBackward
{
public:
  static std::vector<int> make_default_signals()
  {
    const int posix_signals[] = {
      // Signals for which the default action is "Core".
      SIGABRT,  // Abort signal from abort(3)
      SIGBUS,   // Bus error (bad memory access)
      SIGFPE,   // Floating point exception
      SIGILL,   // Illegal Instruction
      SIGIOT,   // IOT trap. A synonym for SIGABRT
      SIGQUIT,  // Quit from keyboard
      SIGSEGV,  // Invalid memory reference
      SIGSYS,   // Bad argument to routine (SVr4)
      SIGTRAP,  // Trace/breakpoint trap
      SIGXCPU,  // CPU time limit exceeded (4.2BSD)
      SIGXFSZ,  // File size limit exceeded (4.2BSD)
#if defined(BACKWARD_SYSTEM_DARWIN)
      SIGEMT,  // emulation instruction executed
#endif
    };
    return std::vector<int>(posix_signals,
                            posix_signals + sizeof posix_signals / sizeof posix_signals[0]);
  }

  FetchBackward(const std::vector<int> &posix_signals = make_default_signals())
    : _loaded(false)
  {
    bool success = true;

    const size_t stack_size = 1024 * 1024 * 8;
    _stack_content.reset(static_cast<char *>(malloc(stack_size)));
    if (_stack_content)
    {
      stack_t ss;
      ss.ss_sp    = _stack_content.get();
      ss.ss_size  = stack_size;
      ss.ss_flags = 0;
      if (sigaltstack(&ss, nullptr) < 0)
      {
        success = false;
      }
    }
    else
    {
      success = false;
    }

    for (size_t i = 0; i < posix_signals.size(); ++i)
    {
      struct sigaction action;
      memset(&action, 0, sizeof action);
      action.sa_flags = static_cast<int>(SA_SIGINFO | SA_ONSTACK | SA_NODEFER | SA_RESETHAND);
      sigfillset(&action.sa_mask);
      sigdelset(&action.sa_mask, posix_signals[i]);
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
      action.sa_sigaction = &sig_handler;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

      int r = sigaction(posix_signals[i], &action, nullptr);
      if (r < 0)
        success = false;
    }

    _loaded = success;
  }

  bool loaded() const
  {
    return _loaded;
  }

  static void handleSignal(int, siginfo_t *info, void *_ctx)
  {
    ucontext_t *uctx = static_cast<ucontext_t *>(_ctx);

    backward::StackTrace st;
    void *               error_addr = nullptr;
#ifdef REG_RIP  // x86_64
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext.gregs[REG_RIP]);
#elif defined(REG_EIP)  // x86_32
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext.gregs[REG_EIP]);
#elif defined(__arm__)
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext.arm_pc);
#elif defined(__aarch64__)
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext.pc);
#elif defined(__ppc__) || defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__)
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext.regs->nip);
#elif defined(__s390x__)
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext.psw.addr);
#elif defined(__APPLE__) && defined(__x86_64__)
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext->__ss.__rip);
#elif defined(__APPLE__)
    error_addr = reinterpret_cast<void *>(uctx->uc_mcontext->__ss.__eip);
#else
#warning ":/ sorry, ain't know no nothing none not of your architecture!"
#endif
    if (error_addr)
    {
      st.load_from(error_addr, 32);
    }
    else
    {
      st.load_here(32);
    }

    backward::Printer printer;
    printer.address = true;

    std::ostringstream oss;

    printer.print(st, oss);

    FETCH_LOG_ERROR("SIG_HANDLER", oss.str());

#if _XOPEN_SOURCE >= 700 || _POSIX_C_SOURCE >= 200809L
    psiginfo(info, nullptr);
#else
    (void)info;
#endif
  }

private:
  backward::details::handle<char *> _stack_content;
  bool                              _loaded;

#ifdef __GNUC__
  __attribute__((noreturn))
#endif
  static void
  sig_handler(int signo, siginfo_t *info, void *_ctx)
  {
    handleSignal(signo, info, _ctx);

    // try to forward the signal.
    raise(info->si_signo);

    // terminate the process immediately.
    puts("watf? exit");
    _exit(EXIT_FAILURE);
  }
};

#endif  // BACKWARD_SYSTEM_LINUX || BACKWARD_SYSTEM_DARWIN

#ifdef BACKWARD_SYSTEM_UNKNOWN

class FetchBackward
{
public:
  FetchBackward(const std::vector<int> & = std::vector<int>())
  {}
  bool init()
  {
    return false;
  }
  bool loaded()
  {
    return false;
  }
};

#endif  // BACKWARD_SYSTEM_UNKNOWN

}  // namespace log
}  // namespace fetch

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
