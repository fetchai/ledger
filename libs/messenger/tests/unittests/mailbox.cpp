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

#include "gtest/gtest.h"
#include "shared_functions.hpp"

TEST(MessengerMailboxTest, BasicRegisteringUnregistering)
{
  auto server = NewServerWithFakeMailbox(0);

  // Registering a mailbox for every other messenger
  std::vector<std::shared_ptr<Messenger>> messengers;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (uint64_t i = 0; i < 10; ++i)
  {
    auto messenger = NewMessenger(1337);
    messenger->messenger->Register((i & 1) == 0);
    messengers.push_back(messenger);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(server->mailbox.unregistered_messengers, 0);
  EXPECT_EQ(server->mailbox.registered_messengers, 5);

  // Unregistering
  for (auto const &messenger : messengers)
  {
    messenger->messenger->Unregister();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  EXPECT_EQ(server->mailbox.unregistered_messengers, 10);
  EXPECT_EQ(server->mailbox.registered_messengers, 5);

  // Teardown
  while (!messengers.empty())
  {
    auto messenger = messengers.back();
    messengers.pop_back();
    messenger->messenger_muddle->Stop();
    messenger->network_manager.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Shutting down
  server->http.Stop();
  server->messenger_muddle->Stop();
  server->network_manager.Stop();
}

TEST(MessengerMailboxTest, BilateralCommsMailbox)
{
  auto server = NewServer(0);

  // Testing mailbox.
  auto messenger1 = NewMessenger(1337);
  auto messenger2 = NewMessenger(1337);

  messenger1->messenger->Register(true);
  messenger2->messenger->Register(true);

  std::deque<Message> sent_messages1;
  std::deque<Message> sent_messages2;

  for (uint64_t i = 0; i < 10; ++i)
  {
    if ((i & 1) != 0)
    {
      Message msg;
      msg.from.node      = server->mail_muddle->GetAddress();
      msg.from.messenger = messenger1->messenger_muddle->GetAddress();
      msg.to.node        = server->mail_muddle->GetAddress();
      msg.to.messenger   = messenger2->messenger_muddle->GetAddress();

      messenger1->messenger->SendMessage(msg);
      sent_messages2.push_back(msg);
    }
    else
    {
      Message msg;
      msg.from.node      = server->mail_muddle->GetAddress();
      msg.from.messenger = messenger2->messenger_muddle->GetAddress();
      msg.to.node        = server->mail_muddle->GetAddress();
      msg.to.messenger   = messenger1->messenger_muddle->GetAddress();

      messenger2->messenger->SendMessage(msg);
      sent_messages1.push_back(msg);
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  auto messages1          = server->mailbox.GetMessages(messenger1->messenger_muddle->GetAddress());
  auto messages2          = server->mailbox.GetMessages(messenger2->messenger_muddle->GetAddress());
  auto recevied_messages1 = messenger1->messenger->GetMessages(200);
  auto recevied_messages2 = messenger2->messenger->GetMessages(200);

  EXPECT_EQ(ToSet(messages1), ToSet(recevied_messages1));
  EXPECT_EQ(ToSet(messages1), ToSet(sent_messages1));

  EXPECT_EQ(ToSet(messages2), ToSet(recevied_messages2));
  EXPECT_EQ(ToSet(messages2), ToSet(sent_messages2));

  // Shutting down
  server->http.Stop();
  server->messenger_muddle->Stop();
  server->mail_muddle->Stop();
  server->network_manager.Stop();
}

TEST(MessengerMailboxTest, MessagesRouting)
{
#define NETWORK_LENGTH 5
  std::vector<std::shared_ptr<Server>> servers;
  // Creating servers
  for (uint16_t i = 0; i < NETWORK_LENGTH; ++i)
  {
    servers.emplace_back(NewServer(i));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Connecting servers in a line
  for (uint16_t i = 0; i < NETWORK_LENGTH; ++i)
  {
    auto &a = servers[i];
    for (uint16_t j = 0; j < NETWORK_LENGTH; ++j)
    {
      if (i == j)
      {
        continue;
      }
      a->mail_muddle->ConnectTo("",
                                fetch::network::Uri("tcp://127.0.0.1:" + std::to_string(6500 + j)));
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1000 * NETWORK_LENGTH));

  // Creating one messenger per server
  std::vector<std::shared_ptr<Messenger>> messengers;
  for (uint16_t i = 0; i < NETWORK_LENGTH; ++i)
  {
    auto messenger = NewMessenger(static_cast<uint16_t>(1337 + i));
    messenger->messenger->Register(true);
    messengers.push_back(messenger);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100 * NETWORK_LENGTH));

  // Sending a message from every messenger to every other messenger
  for (uint16_t i = 0; i < NETWORK_LENGTH; ++i)
  {
    std::string prefix = "Hello from " + std::to_string(i) + " to ";
    auto &      from   = messengers[i];

    for (uint16_t j = 0; j < NETWORK_LENGTH; ++j)
    {
      auto &to_server = servers[j];
      auto &to        = messengers[j];

      std::string message = prefix + std::to_string(j);
      Message     msg;
      // Delibrately left out     msg.from.node      = server->mail_muddle->GetAddress();
      // Delibrately left out     msg.from.messenger = from->messenger_muddle->GetAddress();
      msg.to.node      = to_server->mail_muddle->GetAddress();
      msg.to.messenger = to->messenger_muddle->GetAddress();

      // Sending message
      from->messenger->SendMessage(msg);
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1000 * NETWORK_LENGTH * NETWORK_LENGTH));

  for (auto &messenger : messengers)
  {
    auto recevied_messages = messenger->messenger->GetMessages(400);
    EXPECT_EQ(recevied_messages.size(), NETWORK_LENGTH);
  }

  // Shutting down
  for (auto &server : servers)
  {
    server->http.Stop();
    server->messenger_muddle->Stop();
    server->mail_muddle->Stop();
    server->network_manager.Stop();
  }
}
