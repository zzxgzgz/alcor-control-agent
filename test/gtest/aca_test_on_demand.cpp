#include "gtest/gtest.h"
#define private public
// #include "aca_arp_responder.h"
// #include "aca_net_config.h"
// #include "aca_ovs_l2_programmer.h"
// #include "aca_comm_mgr.h"
// #include "aca_util.h"
#include "goalstate.pb.h"
// #include "aca_ovs_control.h"
#include "aca_grpc.h"
#include <uuid/uuid.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include "aca_log.h"

extern GoalStateProvisionerImpl *g_grpc_server;

TEST(on_demand_testcases, DISABLED_grpc_client_connection)
{
  HostRequest HostRequest_builder;
  HostRequest_ResourceStateRequest *new_state_requests =
          HostRequest_builder.add_state_requests();
  HostRequestReply hostRequestReply;

  uuid_t uuid;
  uuid_generate_time(uuid);
  char uuid_str[37];
  uuid_unparse_lower(uuid, uuid_str);
  uint tunnel_id = 1;
  new_state_requests->set_request_id(uuid_str);
  new_state_requests->set_tunnel_id(tunnel_id);
  new_state_requests->set_source_ip("10.0.0.2");
  new_state_requests->set_source_port("123");
  new_state_requests->set_destination_ip("10.0.0.3");
  new_state_requests->set_destination_port("123");
  //   new_state_requests->set_protocol(protocol);
  new_state_requests->set_ethertype(EtherType::IPV4);

  hostRequestReply = g_grpc_server->RequestGoalStates(&HostRequest_builder);

  hostRequestReply.operation_status();
  ACA_LOG_INFO("Got this HostRequestReply: \n[%s]\n", hostRequestReply.GetTypeName());
}