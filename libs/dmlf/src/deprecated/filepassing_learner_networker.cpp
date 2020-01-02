//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/serializers/base_types.hpp"
#include "dmlf/deprecated/filepassing_learner_networker.hpp"

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

#include "dmlf/deprecated/update_interface.hpp"
#include <cstdio>  //for remove( ) and rename( )

#include <chrono>

using namespace std::chrono_literals;

namespace fetch {
namespace dmlf {

void deprecated_FilepassingLearnerNetworker::SetName(const std::string &name)
{
  name_  = name;
  dir_   = ProcessNameToTargetDir(name);
  auto r = system((std::string("mkdir -vp ") + dir_).c_str());  // NOLINT
  if (r != 0)
  {
    std::cerr << "mkdir failed" << std::endl;
  }

  auto names = GetUpdateNames();
  for (auto const &name_to_unlink : names)
  {
    ::unlink(name_to_unlink.c_str());
  }
  running_ = true;
  watcher_ =
      std::make_shared<std::thread>(&deprecated_FilepassingLearnerNetworker::CheckUpdates, this);
}

void deprecated_FilepassingLearnerNetworker::CheckUpdates()
{
  while (running_)
  {
    std::this_thread::sleep_for(100ms);
    auto pendings = GetUpdateNames();

    if (pendings.empty())
    {
      continue;
    }

    // read the files dropped into our dir by other processes.
    for (auto const &filename : pendings)
    {
      processed_updates_.insert(filename);
      Bytes         b;
      std::ifstream inp(filename.c_str());
      std::string   str((std::istreambuf_iterator<char>(inp)), std::istreambuf_iterator<char>());
      inp.close();
      // Got the message, deliver it to the queue internally.
      NewMessage(Bytes{str});
    }
  }
}

deprecated_FilepassingLearnerNetworker::~deprecated_FilepassingLearnerNetworker()
{
  running_ = false;
  watcher_->join();
}

std::string deprecated_FilepassingLearnerNetworker::ProcessNameToTargetDir(const std::string &name)
{
  return std::string("/tmp/deprecated_FilepassingLearnerNetworker/") + name + "/";
}

void deprecated_FilepassingLearnerNetworker::AddPeers(Peers new_peers)
{
  for (auto const &peer : new_peers)
  {
    if (peer != name_)
    {
      peers_.emplace_back(peer);
    }
  }
}

void deprecated_FilepassingLearnerNetworker::ClearPeers()
{
  peers_.clear();
}

void deprecated_FilepassingLearnerNetworker::PushUpdate(deprecated_UpdateInterfacePtr const &update)
{
  auto indexes = alg_->GetNextOutputs();
  auto data    = update->Serialise();

  for (auto ind : indexes)
  {
    auto t = peers_[ind];
    Transmit(t, data);
  }
}

void deprecated_FilepassingLearnerNetworker::Transmit(const std::string &target, Bytes const &data)
{
  static int filecounter = 0;
  auto       target_dir  = ProcessNameToTargetDir(target);
  auto       filename    = name_ + "-" + std::to_string(filecounter++);
  auto       tmpfilename = std::string("tmp_") + name_ + "-" + std::to_string(filecounter++);

  auto filepath    = target_dir + filename;
  auto tmpfilepath = target_dir + tmpfilename;

  std::ofstream outp(tmpfilepath.c_str());
  outp << data;
  outp.close();

  ::rename(tmpfilepath.c_str(), filepath.c_str());
}

std::vector<std::string> deprecated_FilepassingLearnerNetworker::GetUpdateNames() const
{
  std::vector<std::string> r;

  DIR *          dir;
  struct dirent *ent;
  if ((dir = ::opendir(dir_.c_str())) != nullptr)
  {
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != nullptr)
    {

      std::string f(ent->d_name);

      if (f == ".")
      {
        continue;
      }
      if (f == "..")
      {
        continue;
      }

      if (f.substr(0, 4) == "tmp_")
      {
        continue;
      }
      auto fp = dir_ + f;
      if (processed_updates_.find(fp) != processed_updates_.end())
      {
        continue;
      }
      r.emplace_back(fp);
    }
    closedir(dir);
  }
  else
  {
    /* could not open directory */
    throw std::length_error("Updates list failed.");
  }

  return r;
}

}  // namespace dmlf
}  // namespace fetch
