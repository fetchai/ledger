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

#include "network/service/client_interface.hpp"

#include <algorithm>

namespace fetch {
namespace service {

void ServiceClientInterface::ProcessRPCResult(network::MessageBuffer const &msg,
                                              service::SerializerType &     params)
{
  // extract the promise counter (or request number)
  PromiseCounter id;
  params >> id;

  // look up the promise
  Promise p = ExtractPromise(id);

  // There is a chance that this message will not belong to this client. In this case we will
  // simply ignore the message
  if (p)
  {
    auto ret = msg.SubArray(params.tell(), msg.size() - params.tell());
    p->Fulfill(ret);
  }
}

bool ServiceClientInterface::ProcessServerMessage(network::MessageBuffer const &msg)
{
  bool ret = true;

  SerializerType params(msg);

  ServiceClassificationType type;
  params >> type;

  FETCH_LOG_TRACE(LOGGING_NAME, "ProcessServerMessage: type: ", type, " msg: 0x", msg.ToHex());

  if (type == SERVICE_RESULT)
  {
    ProcessRPCResult(msg, params);
  }
  else if (type == SERVICE_ERROR)
  {
    PromiseCounter id;
    params >> id;

    serializers::SerializableException e;
    params >> e;

    // look up the promise and fail it
    Promise p = ExtractPromise(id);

    if (!p)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Attempted to look up a network promise but it was already deleted");
    }
    else
    {
      p->Fail(e);
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Binning promise ", id,
                    " due to finishing delivering the response (error)");
  }
  else
  {
    PromiseCounter id;
    params >> id;
    FETCH_LOG_WARN(LOGGING_NAME, " type not recognised ", type, "  promise=", id);
    ret = false;
  }

  return ret;
}

void ServiceClientInterface::AddPromise(Promise const &promise)
{
  FETCH_LOCK(promises_mutex_);
  promises_[promise->id()] = promise;
}

Promise ServiceClientInterface::ExtractPromise(PromiseCounter id)
{
  Promise promise{};

  FETCH_LOCK(promises_mutex_);
  auto it = promises_.find(id);
  if (it != promises_.end())
  {
    promise = it->second;
    promises_.erase(it);
  }

  return promise;
}

void ServiceClientInterface::RemovePromise(PromiseCounter id)
{
  FETCH_LOCK(promises_mutex_);
  promises_.erase(id);
}

}  // namespace service
}  // namespace fetch
