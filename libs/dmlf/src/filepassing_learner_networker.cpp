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

#include "core/serializers/base_types.hpp"
#include "dmlf/filepassing_learner_networker.hpp"

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

#include "dmlf/iupdate.hpp"
#include <stdio.h>  //for remove( ) and rename( )

namespace fetch {
namespace dmlf {

FilepassingLearnerNetworker::FilepassingLearnerNetworker()
{}

void FilepassingLearnerNetworker::setName(const std::string &name)
{
  name_  = name;
  dir_   = processNameToTargetDir(name);
  auto r = system((std::string("mkdir -vp ") + dir_).c_str());
  if (r)
  {
    std::cerr << "mkdir failed" << std::endl;
  }

  auto names = getUpdateNames();
  for (auto name : names)
  {
    // std::cout << "UNLINK:: "<< name << std::endl;
    ::unlink(name.c_str());
  }
}

FilepassingLearnerNetworker::~FilepassingLearnerNetworker()
{}

std::string FilepassingLearnerNetworker::processNameToTargetDir(const std::string n)
{
  return std::string("/tmp/FilepassingLearnerNetworker/") + n + "/";
}

void FilepassingLearnerNetworker::addPeers(Peers new_peers)
{
  for (auto peer : new_peers)
  {
    if (peer != name_)
    {
      peers_.push_back(peer);
    }
  }
}

void FilepassingLearnerNetworker::clearPeers()
{
  peers_.clear();
}

void FilepassingLearnerNetworker::pushUpdate(std::shared_ptr<IUpdate> update)
{
  auto indexes = alg->getNextOutputs();
  auto data    = update->serialise();

  for (auto ind : indexes)
  {
    auto t = peers_[ind];
    tx(t, data);
  }
}

void FilepassingLearnerNetworker::tx(const std::string &target, const Intermediate &data)
{
  static int filecounter = 0;
  auto       target_dir  = processNameToTargetDir(target);
  auto       filename    = name_ + "-" + std::to_string(filecounter++);
  auto       tmpfilename = std::string("tmp_") + name_ + "-" + std::to_string(filecounter++);

  auto filepath    = target_dir + filename;
  auto tmpfilepath = target_dir + tmpfilename;

  std::ofstream outp(tmpfilepath.c_str());
  outp << data;
  outp.close();

  ::rename(tmpfilepath.c_str(), filepath.c_str());
}

std::size_t FilepassingLearnerNetworker::getUpdateCount() const
{
  return getUpdateNames().size();
}

std::vector<std::string> FilepassingLearnerNetworker::getUpdateNames() const
{
  std::vector<std::string> r;

  DIR *          dir;
  struct dirent *ent;
  if ((dir = ::opendir(dir_.c_str())) != NULL)
  {
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL)
    {

      std::string f(ent->d_name);

      if (f == ".")
        continue;
      if (f == "..")
        continue;

      if (f.substr(0, 4) == "tmp_")
      {
        continue;
      }
      auto fp = dir_ + f;
      if (processed_updates_.find(fp) != processed_updates_.end())
      {
        continue;
      }
      r.push_back(fp);
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

FilepassingLearnerNetworker::Intermediate FilepassingLearnerNetworker::getUpdateIntermediate()
{
  auto pendings = getUpdateNames();

  if (pendings.empty())
  {
    throw std::length_error("Updates list is already empty.");
  }
  auto filename = pendings[0];
  processed_updates_.insert(filename);
  Intermediate  b;
  std::ifstream inp(filename.c_str());
  std::string   str((std::istreambuf_iterator<char>(inp)), std::istreambuf_iterator<char>());
  inp.close();
  return Intermediate(str);
}

}  // namespace dmlf
}  // namespace fetch
