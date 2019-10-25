#include "main.hpp"

#include "MtCore.hpp"

int main(int argc, char *argv[])
{
  // parse args

  std::string config_file;

  if (argc != 2)
  {
    FETCH_LOG_ERROR("MTCoreApp", "Failed to run binary, because exactly 1 argument ",
                                 "(path to config file) should be passed!");
    return -1;
  }

  config_file = argv[1];

  MtCore myCore;

  if (config_file.empty())
  {
    FETCH_LOG_WARN("MTCoreApp", "Configuration not provided!");
    return 1;
  }


  if (!myCore.configure(config_file, ""))
  {
    FETCH_LOG_WARN("MTCoreApp", "Configuration failed, shutting down...");
    return 1;
  }

  myCore.run();
}
