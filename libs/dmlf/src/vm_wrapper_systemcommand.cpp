#include "dmlf/vm_wrapper_systemcommand.hpp"

#include <unistd.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <fstream>
#include <sys/select.h>
#include <sys/fcntl.h>

namespace fetch {
namespace dmlf {

  using CHARP = char *const;

  static std::vector<std::string> splitString(const std::string &input)
  {
    std::istringstream ss(input);
    std::string token;

    std::vector<std::string> output;
    while(std::getline(ss, token, ',')) {
      output.push_back(token);
    }
    return output;
  }

  void dispatchString(const std::string &data, VmWrapperInterface::OutputHandler func, const std::string &name)
  {
    if (func)
    {
      func(data);
    }
    else
    {
      std::cout << name << ":" << data << std::endl;
    }
  }

  void dispatch(std::string &out, VmWrapperInterface::OutputHandler func, const std::string &name, bool flush)
  {
    while(1)
    {
      auto where = out.find_first_of("\n", 0);
      if (where == std::string::npos)
      {
        if (flush && out.length())
        {
          dispatchString(out, func, name);
          out = "";
        }
        return;
      }
      else
      {
        auto foo = out.substr(0, where);
        out = out.substr(where+1);
        dispatchString(foo, func, name);
      }
    }
  }

  bool slurp(std::string &out, FILE *fp)
  {
    char buf[10000];
    while(true)
    {
      auto r = ::fgets(buf, 9999, fp);
      if (r == 0)
      {
        if (feof(fp))
        {
          return false;
        }
        else
        {
          if (errno!=EAGAIN || errno!=EWOULDBLOCK)
          {
            std::cerr << "fgets - Error:" << strerror(errno) << std::endl;
            return false;
          }
          else
          {
            // come back later
            return true;
          }
        }
      }
      else
      {
        out += std::string(buf);
      }
    }
  }

void VmWrapperSystemcommand::Execute(std::string entrypoint, const Params /*params*/)
{
  status_ = VmWrapperInterface::RUNNING;

  ::pipe(stdin_pipe); // 0 = read end, 1 = write end.
  ::pipe(stderr_pipe);
  ::pipe(stdout_pipe);

  // stdin is the other way round.
  std::swap(stdin_pipe[0], stdin_pipe[1]);

  // Now, ZERO contains the server end, ONE contains the child end.

  int p = ::fork();
  switch(p)
  {
  case -1:
    // error
    ::close(stdin_pipe[0]);
    ::close(stderr_pipe[0]);
    ::close(stdout_pipe[0]);
    ::close(stdin_pipe[0]);
    ::close(stderr_pipe[0]);
    ::close(stdout_pipe[0]);
    std::cerr << "fork - Error:" << strerror(errno) << std::endl;
    return;

  case 0:
    {
      //we are the child.
      ::close(stdin_pipe[0]);
      ::close(stderr_pipe[0]);
      ::close(stdout_pipe[0]);

      ::dup2(stdin_pipe[1],  0);
      ::dup2(stderr_pipe[1], 2);
      ::dup2(stdout_pipe[1], 1);

      ::close(stdin_pipe[1]);
      ::close(stderr_pipe[1]);
      ::close(stdout_pipe[1]);

      auto argv = splitString(command_);
      std::vector<char *> vec_cp;
      vec_cp.reserve(argv.size() + 2);
      //vec_cp.push_back(strdup(entrypoint.c_str()));
      for (auto s : argv)
      {
        vec_cp.push_back(strdup(s.c_str()));
      }
      vec_cp.push_back(NULL);
      ::execv(entrypoint.c_str(), vec_cp.data());

      // OMG! That failed then...
      std::cerr << "exec - Error:" << strerror(errno) << std::endl;
      return;
    }
  default:
    {
      // we are the parent.
      ::close(stdin_pipe[1]);
      ::close(stderr_pipe[1]);
      ::close(stdout_pipe[1]);

      FILE *stdout_fp = fdopen(stdout_pipe[0], "r");
      FILE *stderr_fp = fdopen(stdout_pipe[0], "r");

      {
        int flags = fcntl(stderr_pipe[0], F_GETFL, 0);
        fcntl(stderr_pipe[0], F_SETFL, flags | O_NONBLOCK);
      }
      {
        int flags = fcntl(stdout_pipe[0], F_GETFL, 0);
        fcntl(stdout_pipe[0], F_SETFL, flags | O_NONBLOCK);
      }

      fd_set rfds, xfds;

      std::string err, out;

      auto running = true;

      while(running)
      {
        FD_ZERO(&rfds);
        FD_ZERO(&xfds);

        FD_SET(stdout_pipe[0], &xfds);
        FD_SET(stderr_pipe[0], &xfds);
        FD_SET(stdout_pipe[0], &rfds);
        FD_SET(stderr_pipe[0], &rfds);

        struct timeval to;
        to.tv_sec = 0;
        to.tv_usec = 1000 * 100;

        auto sel = ::select(2, &rfds, 0, &xfds, &to);
        switch(sel)
        {
        case -1:
          std::cerr << "select - Error:" << strerror(errno) << std::endl;
          running = false;
          continue;
        case 0:
        default:

          slurp(out, stdout_fp);
          slurp(err, stderr_fp);

          dispatch(out, oh_, "stdout", false);
          dispatch(err, eh_, "stderr", false);

          // output avail.
          break;
        }

        int exitcode;
        switch(::waitpid(p, &exitcode, WNOHANG))
        {
        case -1:
          std::cerr << "waitpid - Error:" << strerror(errno) << std::endl;
          running = false;
          continue;
        default:
          result = WEXITSTATUS(exitcode);
          running = false;
          break;
        }
      }

      dispatch(out, oh_, "stdout", true);
      dispatch(err, eh_, "stderr", true);

      ::fclose(stdout_fp);
      ::fclose(stderr_fp);

      ::close(stdin_pipe[0]);
      ::close(stderr_pipe[0]);
      ::close(stdout_pipe[0]);
    }
  }

  if (result == 0)
  {
    status_ = VmWrapperInterface::COMPLETED;
  }
  else
  {
    status_ = VmWrapperInterface::FAILED_RUN;
  }
}

void VmWrapperSystemcommand::SetStdout(OutputHandler oh)
{
  oh_ = oh;
}

void VmWrapperSystemcommand::SetStdin(InputHandler ih)
{
  ih_ = ih;
}

void VmWrapperSystemcommand::SetStderr(OutputHandler eh)
{
  eh_ = eh;
}



}
}
