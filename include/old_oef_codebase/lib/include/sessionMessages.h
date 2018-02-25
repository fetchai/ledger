// Company: FETCH.ai
// Author: Nathan Hutton
// Creation: 09/02/18
//
// Defines the messages and control that will be used by the AEA to check that a session still exists between itself and an AEA.
// This is a stand-in for a more fully-fledged system where every message gets an acknowledgement

#pragma once

// Fetch.ai
#include "serialize.h"
#include "messages.h"
#include "uuid.h"
#include "schema.h"

// IsAliveChallenge message/check that a particular endpoint is 'still there'
class IsAliveChallenge
{
 public:
  explicit IsAliveChallenge(uint32_t challenge) : _challenge{challenge} {}

  template <typename Archive>
  explicit IsAliveChallenge(const Archive &ar) : _challenge{ar.getUint("IsAliveChallenge")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("IsAliveChallenge", _challenge);
  }

  uint32_t challenge() const { return _challenge; }

 private:
  uint32_t _challenge;
};

// IsAliveResponse is required in response to a challenge, must return same number
class IsAliveResponse
{
 public:
  explicit IsAliveResponse(uint32_t response) : _response{response} {}

  template <typename Archive>
  explicit IsAliveResponse(const Archive &ar) : _response{ar.getUint("IsAliveResponse")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("IsAliveResponse", _response);
  }

  uint32_t response() const { return _response; }

 private:
  uint32_t _response;
};
