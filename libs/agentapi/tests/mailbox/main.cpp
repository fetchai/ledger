#include "shared_functions.hpp"

#include "gtest/gtest.h"

TEST(AgentMailboxTest, BasicRegisteringUnregistering)
{
  auto server = NewServerWithFakeMailbox(1337, 1338);

  // Registering a mailbox for every other agent
  std::vector<std::shared_ptr<Agent>> agents;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (uint64_t i = 0; i < 10; ++i)
  {
    auto agent = NewAgent(1337);
    agent->agent->Register((i & 1) == 0);
    agents.push_back(agent);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(server->mailbox.unregistered_agents, 0);
  EXPECT_EQ(server->mailbox.registered_agents, 5);

  // Unregistering
  while (!agents.empty())
  {
    auto agent = agents.back();
    agents.pop_back();
    agent->agent->Unregister();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  EXPECT_EQ(server->mailbox.unregistered_agents, 10);
  EXPECT_EQ(server->mailbox.registered_agents, 5);

  // Testing mailbox.
  auto agent1 = NewAgent(1337);
  auto agent2 = NewAgent(1337);

  agent1->agent->Register(true);
  agent2->agent->Register(true);

  std::vector<Message> sent_messages1;
  std::vector<Message> sent_messages2;

  for (uint64_t i = 0; i < 10; ++i)
  {
    if (i & 1)
    {
      Message msg;
      msg.from.agent = agent1->agent_muddle->GetAddress();
      msg.to.agent   = agent2->agent_muddle->GetAddress();
      agent1->agent->SendMessage(msg);
      sent_messages2.push_back(msg);
    }
    else
    {
      Message msg;
      msg.from.agent = agent2->agent_muddle->GetAddress();
      msg.to.agent   = agent1->agent_muddle->GetAddress();
      agent2->agent->SendMessage(msg);
      sent_messages1.push_back(msg);
    }
  }

  auto messages1          = server->mailbox.GetMessages(agent1->agent_muddle->GetAddress());
  auto messages2          = server->mailbox.GetMessages(agent2->agent_muddle->GetAddress());
  auto recevied_messages1 = agent1->agent->GetMessages();
  auto recevied_messages2 = agent2->agent->GetMessages();

  EXPECT_EQ(messages1, recevied_messages1);
  EXPECT_EQ(messages1, sent_messages1);

  EXPECT_EQ(messages2, recevied_messages2);
  EXPECT_EQ(messages2, sent_messages2);
}

TEST(AgentMailboxTest, MessagesRouting)
{
  //  std::vector< Server > server_chain;

  //  auto server = NewServer(1337, 1338);
  EXPECT_TRUE(false);
}

/*
TEST(AgentMailboxTest, DirectDelivery)
{
  EXPECT_TRUE(false);
}

TEST(AgentMailboxTest, MailboxDelivery)
{
  EXPECT_TRUE(false);
}
*/