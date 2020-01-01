#pragma once
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/mcl_dkg.hpp"
#include "muddle/rpc/client.hpp"

#include <string>

namespace fetch {
namespace dkg {

using DKGSerializer = fetch::serializers::MsgPackSerializer;

/**
 * Different messages using in distributed key generation (DKG) protocol.
 * Connections - the cabinet connections the current node has connected directly to
 * Coefficients - contain the broadcast coefficients as strings
 * Shares - contain the secret shares which have been exposed in broadcasts as strings
 * Complaints - contain the set of miners who are being complained against
 */

class DKGMessage
{
public:
  using MuddleAddress = byte_array::ConstByteArray;
  using Coefficient   = crypto::mcl::PublicKey;
  using Share         = crypto::mcl::PrivateKey;
  using CabinetId     = MuddleAddress;

  enum class MessageType : uint8_t
  {
    CONNECTIONS,
    COEFFICIENT,
    SHARE,
    COMPLAINT,
    NOTARISATION_KEY,
    FINAL_STATE
  };

  virtual ~DKGMessage() = default;

  /// @name Getter functions
  /// @{
  MessageType type() const
  {
    return type_;
  }
  /// @}

  virtual DKGSerializer Serialize() const = 0;

protected:
  const MessageType type_;  ///< Type of message of the three listed above

  explicit DKGMessage(MessageType type)
    : type_{type}
  {}
};

class FinalStateMessage : public DKGMessage
{
public:
  using Payload = byte_array::ConstByteArray;

  Payload payload_;

  explicit FinalStateMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::FINAL_STATE}
  {
    serialiser >> payload_;
  }
  explicit FinalStateMessage(Payload const &payload)  // NOLINT
    : DKGMessage{MessageType::FINAL_STATE}
    , payload_{payload}
  {}
  ~FinalStateMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << payload_;
    return serializer;
  }
};

class ConnectionsMessage : public DKGMessage
{
public:
  std::set<MuddleAddress> connections_;

  explicit ConnectionsMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::CONNECTIONS}
  {
    serialiser >> connections_;
  }
  explicit ConnectionsMessage(std::set<MuddleAddress> connections)
    : DKGMessage{MessageType::CONNECTIONS}
    , connections_{std::move(connections)}
  {}
  ~ConnectionsMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << connections_;
    return serializer;
  }
};

class CoefficientsMessage : public DKGMessage
{
  uint8_t                  phase_{};       ///< Phase of state machine that this message is for
  std::vector<Coefficient> coefficients_;  ///< Coefficients as strings

public:
  explicit CoefficientsMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::COEFFICIENT}
  {
    serialiser >> phase_ >> coefficients_;
  }
  CoefficientsMessage(uint8_t phase, std::vector<Coefficient> coeff)
    : DKGMessage{MessageType::COEFFICIENT}
    , phase_{phase}
    , coefficients_{std::move(coeff)}
  {}
  ~CoefficientsMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << phase_ << coefficients_;
    return serializer;
  }

  /// @name Getter functions
  /// @{
  uint8_t phase() const
  {
    return phase_;
  }
  std::vector<Coefficient> const &coefficients() const
  {
    return coefficients_;
  }
  ///@}
};

class SharesMessage : public DKGMessage
{
  uint8_t phase_{};  ///< Phase of state machine that this message is for
  std::unordered_map<CabinetId, std::pair<Share, Share>>
      shares_;  ///< Exposed secret shares for a particular cabinet member
public:
  explicit SharesMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::SHARE}
  {
    serialiser >> phase_ >> shares_;
  }
  SharesMessage(uint8_t phase, std::unordered_map<CabinetId, std::pair<Share, Share>> shares)
    : DKGMessage{MessageType::SHARE}
    , phase_{phase}
    , shares_{std::move(shares)}
  {}
  ~SharesMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << phase_ << shares_;
    return serializer;
  }

  /// @name Getter functions
  /// @{
  uint8_t phase() const
  {
    return phase_;
  }
  std::unordered_map<CabinetId, std::pair<Share, Share>> const &shares() const
  {
    return shares_;
  }
  ///@}
};

class ComplaintsMessage : public DKGMessage
{
  using ComplaintsList = std::set<CabinetId>;
  ComplaintsList complaints_;  ///< Cabinet members that you are complaining against
public:
  // Construction/Destruction
  explicit ComplaintsMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::COMPLAINT}
  {
    serialiser >> complaints_;
  }
  explicit ComplaintsMessage(ComplaintsList complaints)
    : DKGMessage{MessageType::COMPLAINT}
    , complaints_{std::move(complaints)}
  {}
  ~ComplaintsMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << complaints_;
    return serializer;
  }

  /// @name Getter functions
  /// @{
  ComplaintsList const &complaints() const
  {
    return complaints_;
  }
  ///@}
};

class NotarisationKeyMessage : public DKGMessage
{
  using NotarisationKey       = crypto::mcl::PublicKey;
  using ECDSASignature        = byte_array::ConstByteArray;
  using SignedNotarisationKey = std::pair<NotarisationKey, ECDSASignature>;

  SignedNotarisationKey payload_;

public:
  explicit NotarisationKeyMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::NOTARISATION_KEY}
  {
    serialiser >> payload_;
  }
  explicit NotarisationKeyMessage(SignedNotarisationKey payload)
    : DKGMessage{MessageType::NOTARISATION_KEY}
    , payload_{std::move(payload)}
  {}
  ~NotarisationKeyMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << payload_;
    return serializer;
  }

  NotarisationKey PublicKey() const
  {
    return payload_.first;
  }
  ECDSASignature Signature() const
  {
    return payload_.second;
  };
};

class DKGEnvelope
{
  using MessageType = DKGMessage::MessageType;
  using Payload     = byte_array::ConstByteArray;

public:
  DKGEnvelope() = default;
  explicit DKGEnvelope(DKGMessage const &msg)
    : type_{msg.type()}
    , serialisedMessage_{msg.Serialize().data()}
  {}

  std::shared_ptr<DKGMessage> Message() const;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;

private:
  MessageType type_;               ///< Type of message contained in the envelope
  Payload     serialisedMessage_;  ///< Serialised message
};

}  // namespace dkg

namespace serializers {
template <typename D>
struct MapSerializer<dkg::DKGEnvelope, D>
{
public:
  using Type       = dkg::DKGEnvelope;
  using DriverType = D;

  static uint8_t const TYPE    = 1;
  static uint8_t const MESSAGE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &env)
  {
    auto map = map_constructor(2);
    map.Append(TYPE, static_cast<uint8_t>(env.type_));
    map.Append(MESSAGE, env.serialisedMessage_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &env)
  {
    uint8_t type;
    map.ExpectKeyGetValue(TYPE, type);
    map.ExpectKeyGetValue(MESSAGE, env.serialisedMessage_);
    env.type_ = static_cast<dkg::DKGEnvelope::MessageType>(type);
  }
};

}  // namespace serializers

}  // namespace fetch
