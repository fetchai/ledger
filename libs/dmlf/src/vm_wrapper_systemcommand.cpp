#include "dmlf/vm_wrapper_systemcommand.hpp"

#include <unistd.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <fstream>


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

  void VmWrapperSystemcommand::Execute(std::string entrypoint, const Params /*params*/)
{
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
    std::cerr << "Error:" << strerror(errno) << std::endl;
    return;

  case 0:
    {
      std::cout << "FORK=0" << std::endl;
      //we are the child.
      ::close(stdin_pipe[0]);
      ::close(stderr_pipe[0]);
      ::close(stdout_pipe[0]);

      auto argv = splitString(entrypoint);
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
      std::cerr << "Error:" << strerror(errno) << std::endl;
      return;
    }
  default:
    {
      // we are the parent.
      std::cout << "FORK=" << p<< std::endl;
      ::close(stdin_pipe[1]);
      ::close(stderr_pipe[1]);
      ::close(stdout_pipe[1]);

      FILE *fp = fdopen(stdout_pipe[0], "r");
      char buf[10000];

      while(fgets(buf, 10000, fp))
      {
        if (oh_)
        {
          oh_(buf);
        }
        else
        {
          std::cout << buf << std::endl;
        }
      }

      return;
    }
  }
}

void VmWrapperSystemcommand::SetStdout(OutputHandler)
{
}

void VmWrapperSystemcommand::SetStdin(InputHandler)
{
}

void VmWrapperSystemcommand::SetStderr(OutputHandler)
{
}



}
}
