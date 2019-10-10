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

#include "dmlf/vm_persistent.hpp"

namespace fetch {
namespace dmlf {

namespace{
  using Status = vm::IoObserverInterface::Status;
}

Status VmPersistent::Read(const std::string &key, void *data, uint64_t &size) 
{
  auto it = store_.find(key);

  if (it == store_.end()) 
  {
    return Status::PERMISSION_DENIED;
  }

  const auto &buf = it->second;
  if (size < buf.size()) 
  {
    return Status::BUFFER_TOO_SMALL;
  }

  buf.ReadBytes(reinterpret_cast<Buffer::ValueType*>(data), buf.size());
  size = buf.size(); // How much was copied
  
  return Status::OK;
}

Status VmPersistent::Write(const std::string &key, const void *data, uint64_t size)
{
  store_[key] = Buffer(reinterpret_cast<Buffer::ValueType const*>(data), size);
  return Status::OK;
}

Status VmPersistent::Exists(const std::string &key)
{
  auto i = store_.find(key);

  if (i == store_.end())
  {
    return Status::ERROR;
  }
  else
  {
    return Status::OK;
  }
}

VmPersistent VmPersistent::DeepCopy() const
{
  VmPersistent newCopy;

  for (const auto &i : store_)
  {
    const auto &name = i.first;
    const auto &buff = i.second;
    newCopy.store_.emplace(name, buff.Copy());
  }

  return newCopy;
}

}  // namespace dmlf
}  // namespace fetch
