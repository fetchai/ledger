#pragma once

enum AEACommands
{
  GET_INFO = 1,
  CONNECT  = 2
};

enum PeerToPeerCommands
{
  SEND_MESSAGE = 1,
  GET_MESSAGES = 2
};

enum PeerToPeerFeed
{
  NEW_MESSAGE = 1,
  CONNECTING  = 2
};

enum FetchProtocols
{
  AEA_PROTOCOL = 1,
  PEER_TO_PEER = 2
};
