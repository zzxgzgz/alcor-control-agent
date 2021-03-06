// MIT License
// Copyright(c) 2020 Futurewei Cloud
//
//     Permission is hereby granted,
//     free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction,
//     including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons
//     to whom the Software is furnished to do so, subject to the following conditions:
//
//     The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
//     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//     WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "aca_log.h"
#include "aca_util.h"
#include "aca_config.h"
#include "aca_vlan_manager.h"
#include "aca_ovs_l2_programmer.h"
#include "aca_comm_mgr.h"
#include "gtest/gtest.h"
#include "goalstate.pb.h"
#include "aca_ovs_control.h"
#include <unistd.h> /* for getopt */
#include <iostream>
#include <string>
#include <thread>

using namespace std;
using namespace alcor::schema;
using namespace aca_vlan_manager;
using namespace aca_comm_manager;
using namespace aca_net_config;
using namespace aca_ovs_l2_programmer;
using aca_ovs_control::ACA_OVS_Control;

// extern the string and helper functions from aca_test_ovs_util.cpp
extern string project_id;
extern string vpc_id_1;
extern string vpc_id_2;
extern string subnet_id_1;
extern string subnet_id_2;
extern string port_id_1;
extern string port_id_2;
extern string port_id_3;
extern string port_id_4;
extern string port_name_1;
extern string port_name_2;
extern string port_name_3;
extern string port_name_4;
extern string vmac_address_1;
extern string vmac_address_2;
extern string vmac_address_3;
extern string vmac_address_4;
extern string vip_address_1;
extern string vip_address_2;
extern string vip_address_3;
extern string vip_address_4;
extern string remote_ip_1; // for docker network
extern string remote_ip_2; // for docker network
extern NetworkType vxlan_type;
extern string subnet1_gw_ip;
extern string subnet2_gw_ip;
extern string subnet1_gw_mac;
extern string subnet2_gw_mac;
extern bool g_demo_mode;
extern uint neighbors_to_create;
extern thread *ovs_monitor_thread;

extern void aca_test_reset_environment();
extern void aca_test_create_default_port_state(PortState *new_port_states);
extern void aca_test_create_default_subnet_state(SubnetState *new_subnet_states);
extern void aca_test_1_neighbor_CREATE_DELETE(NeighborType input_neighbor_type);
extern void aca_test_1_neighbor_CREATE_DELETE_V2(NeighborType input_neighbor_type);
extern void aca_test_1_port_CREATE_plus_neighbor_CREATE(NeighborType input_neighbor_type);
extern void aca_test_1_port_CREATE_plus_neighbor_CREATE_V2(NeighborType input_neighbor_type);
extern void aca_test_10_neighbor_CREATE(NeighborType input_neighbor_type);
extern void aca_test_10_neighbor_CREATE_V2(NeighborType input_neighbor_type);
extern void aca_test_1_port_CREATE_plus_N_neighbors_CREATE(NeighborType input_neighbor_type,
                                                           uint neighbors_to_create);
extern void
aca_test_1_port_CREATE_plus_N_neighbors_CREATE_V2(NeighborType input_neighbor_type,
                                                  uint neighbors_to_create);

// TODO: setup bridge when br-int is up and br-tun is gone

// TODO: invalid IP

// TODO: invalid mac

// TODO: tunnel ID

// TODO: subnet info not available

// TODO: neighbor invalid IP

// TODO: neighbor invalid mac

//
// Test suite: ovs_l2_test_cases
//
// Testing the ovs dataplane l2 implementation, including port and neighbor configurations
// and test traffics on one machine.
// Note: the two machine tests requires a two machines setup therefore it is DISABLED by default
//   it can be executed by:
//
//     child machine (-p 10.213.43.187 -> IP of parent machine):
//     ./build/tests/aca_tests --gtest_also_run_disabled_tests --gtest_filter=*DISABLED_2_ports_CREATE_test_traffic_CHILD -p 10.213.43.187
//     parent machine (-c 10.213.43.188 -> IP of child machine):
//     ./build/tests/aca_tests --gtest_also_run_disabled_tests --gtest_filter=*DISABLED_2_ports_CREATE_test_traffic_PARENT -c 10.213.43.188
//
TEST(ovs_l2_test_cases, 2_ports_CREATE_test_traffic_plus_neighbor_internal)
{
  // ulong culminative_network_configuration_time = 0;
  ulong not_care_culminative_time;
  int overall_rc = EXIT_SUCCESS;

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // confirm br-int and br-tun bridges are not there
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "br-exists br-int", not_care_culminative_time, overall_rc);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "br-exists br-tun", not_care_culminative_time, overall_rc);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // are the newly created bridges there?
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "br-exists br-int", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "br-exists br-tun", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  string prefix_len = "/24";

  // create two ports (using demo mode) and configure them
  overall_rc = ACA_OVS_L2_Programmer::get_instance().create_port(
          vpc_id_1, port_name_1, vip_address_1 + prefix_len, vmac_address_1, 20,
          not_care_culminative_time);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  overall_rc = ACA_OVS_L2_Programmer::get_instance().create_port(
          vpc_id_1, port_name_2, vip_address_2 + prefix_len, vmac_address_2, 20,
          not_care_culminative_time);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // are the newly created ports there?
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_1 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_2 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test traffic between the two newly created ports
  string cmd_string = "ping -I " + vip_address_1 + " -c1 " + vip_address_2;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);

  cmd_string = "ping -I " + vip_address_2 + " -c1 " + vip_address_1;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);

  // insert neighbor info
  overall_rc = ACA_OVS_L2_Programmer::get_instance().create_or_update_l2_neighbor(
          vip_address_1, vmac_address_1, remote_ip_1, 20, not_care_culminative_time);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // delete neighbor info
  overall_rc = ACA_OVS_L2_Programmer::get_instance().delete_l2_neighbor(
          vip_address_1, vmac_address_1, 20, not_care_culminative_time);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // confirm br-int and br-tun bridges are not there
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "br-exists br-int", not_care_culminative_time, overall_rc);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "br-exists br-tun", not_care_culminative_time, overall_rc);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;
}

TEST(ovs_l2_test_cases, 1_port_CREATE_DELETE)
{
  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  GoalState GoalState_builder;
  PortState *new_port_states = GoalState_builder.add_port_states();
  SubnetState *new_subnet_states = GoalState_builder.add_subnet_states();

  // fill in port state structs
  aca_test_create_default_port_state(new_port_states);

  // fill in subnet state structs
  aca_test_create_default_subnet_state(new_subnet_states);

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // check to ensure the demo port is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_1 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  new_port_states->set_operation_type(OperationType::DELETE);
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the demo port is deleted
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_1 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // clean up

  // free the allocated configurations since we are done with it now
  new_port_states->clear_configuration();
  new_subnet_states->clear_configuration();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;
}

TEST(ovs_l2_test_cases, 1_port_CREATE_DELETE_V2)
{
  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  aca_test_reset_environment();

  GoalStateV2 GoalState_builder;
  PortState new_port_states;
  SubnetState new_subnet_states;

  // fill in port state structs
  aca_test_create_default_port_state(&new_port_states);
  auto &port_states_map = *GoalState_builder.mutable_port_states();
  port_states_map[port_id_1] = new_port_states;

  // fill in subnet state structs
  aca_test_create_default_subnet_state(&new_subnet_states);
  auto &subnet_states_map = *GoalState_builder.mutable_subnet_states();
  subnet_states_map[subnet_id_1] = new_subnet_states;

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // check to ensure the demo port is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_1 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // delete the port
  new_port_states.set_operation_type(OperationType::DELETE);
  GoalState_builder.clear_port_states();
  port_states_map[port_id_1] = new_port_states;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the demo port is deleted
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_1 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // clean up

  // free the allocated configurations since we are done with it now
  new_port_states.clear_configuration();
  new_subnet_states.clear_configuration();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;
}

TEST(ovs_l2_test_cases, 2_ports_CREATE_test_traffic)
{
  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  GoalState GoalState_builder;
  PortState *new_port_states = GoalState_builder.add_port_states();
  SubnetState *new_subnet_states = GoalState_builder.add_subnet_states();

  new_port_states->set_operation_type(OperationType::CREATE);

  // fill in port state structs
  PortConfiguration *PortConfiguration_builder = new_port_states->mutable_configuration();
  PortConfiguration_builder->set_revision_number(1);
  PortConfiguration_builder->set_update_type(UpdateType::FULL);
  PortConfiguration_builder->set_id(port_id_1);

  PortConfiguration_builder->set_vpc_id(vpc_id_1);
  PortConfiguration_builder->set_name(port_name_1);
  PortConfiguration_builder->set_mac_address(vmac_address_1);
  PortConfiguration_builder->set_admin_state_up(true);

  PortConfiguration_FixedIp *FixedIp_builder = PortConfiguration_builder->add_fixed_ips();
  FixedIp_builder->set_subnet_id(subnet_id_1);
  FixedIp_builder->set_ip_address(vip_address_1);

  PortConfiguration_SecurityGroupId *SecurityGroup_builder =
          PortConfiguration_builder->add_security_group_ids();
  SecurityGroup_builder->set_id("1");

  // fill in subnet state structs
  aca_test_create_default_subnet_state(new_subnet_states);

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port 1 in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // check to ensure the port 1 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_1 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // setup the configuration for port 2
  PortConfiguration_builder->set_id(port_id_2);
  PortConfiguration_builder->set_name(port_name_2);
  PortConfiguration_builder->set_mac_address(vmac_address_2);
  FixedIp_builder->set_ip_address(vip_address_2);

  // create a new port 2 in demo mode
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the port 2 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_2 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test traffic between the two newly created ports
  cmd_string = "ping -I " + vip_address_1 + " -c1 " + vip_address_2;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);

  cmd_string = "ping -I " + vip_address_2 + " -c1 " + vip_address_1;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);

  // clean up

  // free the allocated configurations since we are done with it now
  new_port_states->clear_configuration();
  new_subnet_states->clear_configuration();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;
}

TEST(ovs_l2_test_cases, 2_ports_CREATE_test_traffic_V2)
{
  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  GoalStateV2 GoalState_builder;
  PortState new_port_states;
  SubnetState new_subnet_states;

  new_port_states.set_operation_type(OperationType::CREATE);

  // fill in port state structs
  PortConfiguration *PortConfiguration_builder = new_port_states.mutable_configuration();
  PortConfiguration_builder->set_revision_number(1);
  PortConfiguration_builder->set_update_type(UpdateType::FULL);
  PortConfiguration_builder->set_id(port_id_1);

  PortConfiguration_builder->set_vpc_id(vpc_id_1);
  PortConfiguration_builder->set_name(port_name_1);
  PortConfiguration_builder->set_mac_address(vmac_address_1);
  PortConfiguration_builder->set_admin_state_up(true);

  PortConfiguration_FixedIp *FixedIp_builder = PortConfiguration_builder->add_fixed_ips();
  FixedIp_builder->set_subnet_id(subnet_id_1);
  FixedIp_builder->set_ip_address(vip_address_1);

  PortConfiguration_SecurityGroupId *SecurityGroup_builder =
          PortConfiguration_builder->add_security_group_ids();
  SecurityGroup_builder->set_id("1");

  auto &port_states_map = *GoalState_builder.mutable_port_states();
  port_states_map[port_id_1] = new_port_states;

  // fill in subnet state structs
  aca_test_create_default_subnet_state(&new_subnet_states);
  auto &subnet_states_map = *GoalState_builder.mutable_subnet_states();
  subnet_states_map[subnet_id_1] = new_subnet_states;

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port 1 in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // check to ensure the port 1 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_1 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // setup the configuration for port 2
  PortConfiguration_builder->set_id(port_id_2);
  PortConfiguration_builder->set_name(port_name_2);
  PortConfiguration_builder->set_mac_address(vmac_address_2);
  FixedIp_builder->set_ip_address(vip_address_2);

  GoalState_builder.clear_port_states();
  port_states_map[port_id_2] = new_port_states;

  // create a new port 2 in demo mode
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the port 2 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + port_name_2 + " ofport", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test traffic between the two newly created ports
  cmd_string = "ping -I " + vip_address_1 + " -c1 " + vip_address_2;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);

  cmd_string = "ping -I " + vip_address_2 + " -c1 " + vip_address_1;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);

  // clean up

  // free the allocated configurations since we are done with it now
  new_port_states.clear_configuration();
  new_subnet_states.clear_configuration();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;
}

TEST(ovs_l2_test_cases, 10_ports_CREATE)
{
  string port_name_postfix = "11111111-2222-3333-4444-555555555555";
  string ip_address_prefix = "10.0.0.";
  ulong not_care_culminative_time = 0;
  int overall_rc;

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  GoalState GoalState_builder;
  PortState *new_port_states;
  SubnetState *new_subnet_states = GoalState_builder.add_subnet_states();

  const int PORTS_TO_CREATE = 10;

  for (int i = 0; i < PORTS_TO_CREATE; i++) {
    string i_string = std::to_string(i);
    string port_name = i_string + port_name_postfix;

    new_port_states = GoalState_builder.add_port_states();
    new_port_states->set_operation_type(OperationType::CREATE);

    PortConfiguration *PortConfiguration_builder =
            new_port_states->mutable_configuration();
    PortConfiguration_builder->set_revision_number(1);
    PortConfiguration_builder->set_update_type(UpdateType::FULL);
    PortConfiguration_builder->set_id(i_string);

    PortConfiguration_builder->set_vpc_id(vpc_id_1);
    PortConfiguration_builder->set_name(port_name);
    PortConfiguration_builder->set_mac_address(vmac_address_1);
    PortConfiguration_builder->set_admin_state_up(true);

    PortConfiguration_FixedIp *PortIp_builder =
            PortConfiguration_builder->add_fixed_ips();
    PortIp_builder->set_subnet_id(subnet_id_1);
    PortIp_builder->set_ip_address(ip_address_prefix + i_string);

    PortConfiguration_SecurityGroupId *SecurityGroup_builder =
            PortConfiguration_builder->add_security_group_ids();
    SecurityGroup_builder->set_id("1");
  }

  // fill in the subnet state structs
  aca_test_create_default_subnet_state(new_subnet_states);

  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = false;

  GoalStateOperationReply gsOperationReply;
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationReply);
  EXPECT_EQ(overall_rc, EINPROGRESS);

  g_demo_mode = previous_demo_mode;

  // calculate the average latency
  ulong total_port_create_time = 0;

  for (int i = 0; i < PORTS_TO_CREATE; i++) {
    ACA_LOG_DEBUG("Port State(%d) took: %u microseconds or %u milliseconds\n",
                  i, gsOperationReply.operation_statuses(i).state_elapse_time(),
                  us_to_ms(gsOperationReply.operation_statuses(i).state_elapse_time()));

    total_port_create_time += gsOperationReply.operation_statuses(i).state_elapse_time();
  }

  ulong average_port_create_time = total_port_create_time / PORTS_TO_CREATE;

  ACA_LOG_INFO("Average Port Create of %d took: %lu microseconds or %lu milliseconds\n",
               PORTS_TO_CREATE, average_port_create_time,
               us_to_ms(average_port_create_time));

  ACA_LOG_INFO("[TEST METRICS] Elapsed time for message total operation took: %u microseconds or %u milliseconds\n",
               gsOperationReply.message_total_operation_time(),
               us_to_ms(gsOperationReply.message_total_operation_time()));
}

TEST(ovs_l2_test_cases, 10_ports_CREATE_V2)
{
  string port_name_postfix = "11111111-2222-3333-4444-555555555555";
  string ip_address_prefix = "10.0.0.";
  ulong not_care_culminative_time = 0;
  int overall_rc;

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  GoalStateV2 GoalState_builder;
  PortState new_port_states;
  SubnetState new_subnet_states;

  auto &port_states_map = *GoalState_builder.mutable_port_states();

  const int PORTS_TO_CREATE = 10;

  for (int i = 0; i < PORTS_TO_CREATE; i++) {
    string i_string = std::to_string(i);
    string port_name = i_string + port_name_postfix;

    new_port_states.set_operation_type(OperationType::CREATE);

    PortConfiguration *PortConfiguration_builder =
            new_port_states.mutable_configuration();
    PortConfiguration_builder->set_revision_number(1);
    PortConfiguration_builder->set_update_type(UpdateType::FULL);
    PortConfiguration_builder->set_id(i_string);

    PortConfiguration_builder->set_vpc_id(vpc_id_1);
    PortConfiguration_builder->set_name(port_name);
    PortConfiguration_builder->set_mac_address(vmac_address_1);
    PortConfiguration_builder->set_admin_state_up(true);

    PortConfiguration_FixedIp *PortIp_builder =
            PortConfiguration_builder->add_fixed_ips();
    PortIp_builder->set_subnet_id(subnet_id_1);
    PortIp_builder->set_ip_address(ip_address_prefix + i_string);

    PortConfiguration_SecurityGroupId *SecurityGroup_builder =
            PortConfiguration_builder->add_security_group_ids();
    SecurityGroup_builder->set_id("1");

    port_states_map[i_string] = new_port_states;
  }

  // fill in the subnet state structs
  aca_test_create_default_subnet_state(&new_subnet_states);
  auto &subnet_states_map = *GoalState_builder.mutable_subnet_states();
  subnet_states_map[subnet_id_1] = new_subnet_states;

  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = false;

  GoalStateOperationReply gsOperationReply;
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationReply);
  EXPECT_EQ(overall_rc, EINPROGRESS);

  g_demo_mode = previous_demo_mode;

  // calculate the average latency
  ulong total_port_create_time = 0;

  for (int i = 0; i < PORTS_TO_CREATE; i++) {
    ACA_LOG_DEBUG("Port State(%d) took: %u microseconds or %u milliseconds\n",
                  i, gsOperationReply.operation_statuses(i).state_elapse_time(),
                  us_to_ms(gsOperationReply.operation_statuses(i).state_elapse_time()));

    total_port_create_time += gsOperationReply.operation_statuses(i).state_elapse_time();
  }

  ulong average_port_create_time = total_port_create_time / PORTS_TO_CREATE;

  ACA_LOG_INFO("Average Port Create of %d took: %lu microseconds or %lu milliseconds\n",
               PORTS_TO_CREATE, average_port_create_time,
               us_to_ms(average_port_create_time));

  ACA_LOG_INFO("[TEST METRICS] Elapsed time for message total operation took: %u microseconds or %u milliseconds\n",
               gsOperationReply.message_total_operation_time(),
               us_to_ms(gsOperationReply.message_total_operation_time()));
}

TEST(ovs_l2_test_cases, 1_l2_neighbor_CREATE_DELETE)
{
  aca_test_1_neighbor_CREATE_DELETE(NeighborType::L2);
}

TEST(ovs_l2_test_cases, 1_l2_neighbor_CREATE_DELETE_V2)
{
  aca_test_1_neighbor_CREATE_DELETE_V2(NeighborType::L2);
}

TEST(ovs_l2_test_cases, 1_port_CREATE_plus_l2_neighbor_CREATE)
{
  aca_test_1_port_CREATE_plus_neighbor_CREATE(NeighborType::L2);
}

TEST(ovs_l2_test_cases, 10_l2_neighbor_CREATE)
{
  aca_test_10_neighbor_CREATE(NeighborType::L2);
}

TEST(ovs_l2_test_cases, 1_port_CREATE_plus_10_l2_neighbor_CREATE)
{
  // unset debug mode
  bool previous_debug_mode = g_debug_mode;
  // g_debug_mode = false;

  aca_test_1_port_CREATE_plus_N_neighbors_CREATE(NeighborType::L2, neighbors_to_create);

  // restore debug mode
  g_debug_mode = previous_debug_mode;
}

TEST(ovs_l2_test_cases, DISABLED_2_ports_CREATE_test_traffic_PARENT)
{
  string two_port_vmac_address_1 = "fa:16:3e:d7:f2:6a";
  string two_port_vmac_address_2 = "fa:16:3e:d7:f2:6b";
  string two_port_vmac_address_3 = "fa:16:3e:d7:f2:6c";
  string two_port_vmac_address_4 = "fa:16:3e:d7:f2:6d";

  string two_port_vip_address_1 = "11.0.0.105";
  string two_port_vip_address_2 = "11.0.1.106";
  string two_port_vip_address_3 = "11.0.0.107";
  string two_port_vip_address_4 = "11.0.1.108";

  string two_port_vpc_id_1 = "2b08a5bc-b718-11ea-b3de-111111111112";
  string two_port_vpc_id_2 = "2b08a5bc-b718-11ea-b3de-222222222223";

  string two_port_port_id_1 = "11111111-b718-11ea-b3de-111111111112";
  string two_port_port_id_2 = "12222222-b718-11ea-b3de-111111111113";
  string two_port_port_id_3 = "13333333-b718-11ea-b3de-111111111114";
  string two_port_port_id_4 = "14444444-b718-11ea-b3de-111111111115";

  string two_port_port_name_1 = aca_get_port_name(two_port_port_id_1);
  string two_port_port_name_2 = aca_get_port_name(two_port_port_id_2);
  string two_port_port_name_3 = aca_get_port_name(two_port_port_id_3);
  string two_port_port_name_4 = aca_get_port_name(two_port_port_id_4);

  string two_port_subnet_id_1 = "27330ae4-b718-11ea-b3df-111111111113";
  string two_port_subnet_id_2 = "27330ae4-b718-11ea-b3df-222222222224";

  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  ACA_Vlan_Manager::get_instance().clear_all_data();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // monitor br-tun for arp request message
  ovs_monitor_thread =
          new thread(bind(&ACA_OVS_Control::monitor,
                          &ACA_OVS_Control::get_instance(), "br-tun", "resume"));
  ovs_monitor_thread->detach();

  GoalState GoalState_builder;
  PortState *new_port_states = GoalState_builder.add_port_states();
  SubnetState *new_subnet_states = GoalState_builder.add_subnet_states();

  new_port_states->set_operation_type(OperationType::CREATE);

  // fill in port state structs for port 1
  PortConfiguration *PortConfiguration_builder = new_port_states->mutable_configuration();
  PortConfiguration_builder->set_revision_number(2);
  PortConfiguration_builder->set_update_type(UpdateType::FULL);
  PortConfiguration_builder->set_id(two_port_port_id_1);

  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  PortConfiguration_builder->set_name(two_port_port_name_1);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_1);
  PortConfiguration_builder->set_admin_state_up(true);

  PortConfiguration_FixedIp *FixedIp_builder = PortConfiguration_builder->add_fixed_ips();
  FixedIp_builder->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder->set_ip_address(two_port_vip_address_1);

  PortConfiguration_SecurityGroupId *SecurityGroup_builder =
          PortConfiguration_builder->add_security_group_ids();
  SecurityGroup_builder->set_id("2");

  // fill in subnet state structs
  new_subnet_states->set_operation_type(OperationType::INFO);

  SubnetConfiguration *SubnetConiguration_builder =
          new_subnet_states->mutable_configuration();
  SubnetConiguration_builder->set_revision_number(2);
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_1);
  SubnetConiguration_builder->set_id(two_port_subnet_id_1);
  SubnetConiguration_builder->set_cidr("11.0.0.0/24");
  SubnetConiguration_builder->set_tunnel_id(21);

  // add a new neighbor state with CREATE
  NeighborState *new_neighbor_states = GoalState_builder.add_neighbor_states();
  new_neighbor_states->set_operation_type(OperationType::CREATE);

  // fill in neighbor state structs for port 3
  NeighborConfiguration *NeighborConfiguration_builder =
          new_neighbor_states->mutable_configuration();
  NeighborConfiguration_builder->set_revision_number(2);

  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  NeighborConfiguration_builder->set_id(two_port_port_id_3);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_3);
  NeighborConfiguration_builder->set_host_ip_address(remote_ip_2);

  NeighborConfiguration_FixedIp *FixedIp_builder2 =
          NeighborConfiguration_builder->add_fixed_ips();
  FixedIp_builder2->set_neighbor_type(NeighborType::L2);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder2->set_ip_address(two_port_vip_address_3);

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port 1 + port 3 neighbor in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // check to ensure the port 1 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_1 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // setup the configuration for port 2
  PortConfiguration_builder->set_id(two_port_port_id_2);
  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  PortConfiguration_builder->set_name(two_port_port_name_2);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_2);
  FixedIp_builder->set_ip_address(two_port_vip_address_2);
  FixedIp_builder->set_subnet_id(two_port_subnet_id_2);

  // fill in subnet state structs
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_2);
  SubnetConiguration_builder->set_id(two_port_subnet_id_2);
  SubnetConiguration_builder->set_cidr("11.0.1.0/24");
  SubnetConiguration_builder->set_tunnel_id(31);

  // fill in neighbor state structs for port 4
  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  NeighborConfiguration_builder->set_id(two_port_port_id_4);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_4);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_2);
  FixedIp_builder2->set_ip_address(two_port_vip_address_4);

  // create a new port 2 + port 4 neighbor in demo mode
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the port 2 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_2 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_LOG_INFO("-------- After setting up, printout ifconfig: %d -------------\n", overall_rc);

  overall_rc = Aca_Net_Config::get_instance().execute_system_command("ifconfig");

  // test traffic between the two newly created ports
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_2;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_1;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test valid traffic from parent to child
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test invalid traffic from parent to child with different subnets
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // just clearing the port states and reuse the rest GoalState_builder
  new_port_states->clear_configuration();
  GoalState_builder.clear_port_states();

  // delete port 4 as L2 neighbor
  new_neighbor_states->set_operation_type(OperationType::DELETE);
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  ACA_LOG_INFO("-------- Delete port 4 was succcessful: %d -------------\n", overall_rc);

  ACA_LOG_INFO("-------- After deleting port 4, printout ifconfig: %d -------------\n",
               overall_rc);

  overall_rc = Aca_Net_Config::get_instance().execute_system_command("ifconfig");

  overall_rc = EXIT_SUCCESS;

  // should not be able to ping port 4 anymore
  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // but still have to ping port 3 since it is not deleted
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // clean up

  // free the allocated configurations since we are done with it now
  new_neighbor_states->clear_configuration();
  new_subnet_states->clear_configuration();
}

TEST(ovs_l2_test_cases, DISABLED_2_ports_CREATE_test_traffic_CHILD)
{
  string two_port_vmac_address_1 = "fa:16:3e:d7:f2:6a";
  string two_port_vmac_address_2 = "fa:16:3e:d7:f2:6b";
  string two_port_vmac_address_3 = "fa:16:3e:d7:f2:6c";
  string two_port_vmac_address_4 = "fa:16:3e:d7:f2:6d";

  string two_port_vip_address_1 = "11.0.0.105";
  string two_port_vip_address_2 = "11.0.1.106";
  string two_port_vip_address_3 = "11.0.0.107";
  string two_port_vip_address_4 = "11.0.1.108";

  string two_port_vpc_id_1 = "2b08a5bc-b718-11ea-b3de-111111111112";
  string two_port_vpc_id_2 = "2b08a5bc-b718-11ea-b3de-222222222223";

  string two_port_port_id_1 = "11111111-b718-11ea-b3de-111111111112";
  string two_port_port_id_2 = "12222222-b718-11ea-b3de-111111111113";
  string two_port_port_id_3 = "13333333-b718-11ea-b3de-111111111114";
  string two_port_port_id_4 = "14444444-b718-11ea-b3de-111111111115";

  string two_port_port_name_1 = aca_get_port_name(two_port_port_id_1);
  string two_port_port_name_2 = aca_get_port_name(two_port_port_id_2);
  string two_port_port_name_3 = aca_get_port_name(two_port_port_id_3);
  string two_port_port_name_4 = aca_get_port_name(two_port_port_id_4);

  string two_port_subnet_id_1 = "27330ae4-b718-11ea-b3df-111111111113";
  string two_port_subnet_id_2 = "27330ae4-b718-11ea-b3df-222222222224";

  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  ACA_Vlan_Manager::get_instance().clear_all_data();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // monitor br-tun for arp request message
  ovs_monitor_thread =
          new thread(bind(&ACA_OVS_Control::monitor,
                          &ACA_OVS_Control::get_instance(), "br-tun", "resume"));

  GoalState GoalState_builder;
  PortState *new_port_states = GoalState_builder.add_port_states();
  SubnetState *new_subnet_states = GoalState_builder.add_subnet_states();

  new_port_states->set_operation_type(OperationType::CREATE);

  // fill in port state structs for port 3
  PortConfiguration *PortConfiguration_builder = new_port_states->mutable_configuration();
  PortConfiguration_builder->set_revision_number(2);
  PortConfiguration_builder->set_update_type(UpdateType::FULL);
  PortConfiguration_builder->set_id(two_port_port_id_3);

  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  PortConfiguration_builder->set_name(two_port_port_name_3);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_3);
  PortConfiguration_builder->set_admin_state_up(true);

  PortConfiguration_FixedIp *FixedIp_builder = PortConfiguration_builder->add_fixed_ips();
  FixedIp_builder->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder->set_ip_address(two_port_vip_address_3);

  PortConfiguration_SecurityGroupId *SecurityGroup_builder =
          PortConfiguration_builder->add_security_group_ids();
  SecurityGroup_builder->set_id("2");

  // fill in subnet state structs
  new_subnet_states->set_operation_type(OperationType::INFO);

  SubnetConfiguration *SubnetConiguration_builder =
          new_subnet_states->mutable_configuration();
  SubnetConiguration_builder->set_revision_number(2);
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_1);
  SubnetConiguration_builder->set_id(two_port_subnet_id_1);
  SubnetConiguration_builder->set_cidr("11.0.0.0/24");
  SubnetConiguration_builder->set_tunnel_id(21);

  // add a new neighbor state with CREATE
  NeighborState *new_neighbor_states = GoalState_builder.add_neighbor_states();
  new_neighbor_states->set_operation_type(OperationType::CREATE);

  // fill in neighbor state structs for port 1
  NeighborConfiguration *NeighborConfiguration_builder =
          new_neighbor_states->mutable_configuration();
  NeighborConfiguration_builder->set_revision_number(2);

  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  NeighborConfiguration_builder->set_id(two_port_port_id_1);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_1);
  NeighborConfiguration_builder->set_host_ip_address(remote_ip_1);

  NeighborConfiguration_FixedIp *FixedIp_builder2 =
          NeighborConfiguration_builder->add_fixed_ips();
  FixedIp_builder2->set_neighbor_type(NeighborType::L2);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder2->set_ip_address(two_port_vip_address_1);

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port 3 + port 1 neighbor in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // check to ensure the port 3 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_3 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // setup the configuration for port 4
  PortConfiguration_builder->set_id(two_port_port_id_4);
  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  PortConfiguration_builder->set_name(two_port_port_name_4);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_4);
  FixedIp_builder->set_ip_address(two_port_vip_address_4);
  FixedIp_builder->set_subnet_id(two_port_subnet_id_2);

  // fill in subnet state structs
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_2);
  SubnetConiguration_builder->set_id(two_port_subnet_id_2);
  SubnetConiguration_builder->set_cidr("11.0.1.0/24");
  SubnetConiguration_builder->set_tunnel_id(31);

  // fill in port state structs for NEIGHBOR_CREATE_UPDATE for port 2
  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  NeighborConfiguration_builder->set_id(two_port_port_id_2);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_2);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_2);
  FixedIp_builder2->set_ip_address(two_port_vip_address_2);

  // create a new port 4 + port 2 neighbor in demo mode
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the port 4 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_4 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test traffic between the two newly created ports
  cmd_string = "ping -I " + two_port_vip_address_3 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_4 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // clean up

  // free the allocated configurations since we are done with it now
  new_port_states->clear_configuration();
  new_neighbor_states->clear_configuration();
  new_subnet_states->clear_configuration();

  // wait for parent to ping child
  ovs_monitor_thread->join();

  // not deleting br-int and br-tun bridges so that parent can ping the two new ports
}

TEST(ovs_l2_test_cases, DISABLED_2_ports_CREATE_test_traffic_PARENT_V2)
{
  string two_port_vmac_address_1 = "fa:16:3e:d7:f2:6a";
  string two_port_vmac_address_2 = "fa:16:3e:d7:f2:6b";
  string two_port_vmac_address_3 = "fa:16:3e:d7:f2:6c";
  string two_port_vmac_address_4 = "fa:16:3e:d7:f2:6d";

  string two_port_vip_address_1 = "11.0.0.105";
  string two_port_vip_address_2 = "11.0.1.106";
  string two_port_vip_address_3 = "11.0.0.107";
  string two_port_vip_address_4 = "11.0.1.108";

  string two_port_vpc_id_1 = "2b08a5bc-b718-11ea-b3de-111111111112";
  string two_port_vpc_id_2 = "2b08a5bc-b718-11ea-b3de-222222222223";

  string two_port_port_id_1 = "11111111-b718-11ea-b3de-111111111112";
  string two_port_port_id_2 = "12222222-b718-11ea-b3de-111111111113";
  string two_port_port_id_3 = "13333333-b718-11ea-b3de-111111111114";
  string two_port_port_id_4 = "14444444-b718-11ea-b3de-111111111115";

  string two_port_port_name_1 = aca_get_port_name(two_port_port_id_1);
  string two_port_port_name_2 = aca_get_port_name(two_port_port_id_2);
  string two_port_port_name_3 = aca_get_port_name(two_port_port_id_3);
  string two_port_port_name_4 = aca_get_port_name(two_port_port_id_4);

  string two_port_subnet_id_1 = "27330ae4-b718-11ea-b3df-111111111113";
  string two_port_subnet_id_2 = "27330ae4-b718-11ea-b3df-222222222224";

  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  ACA_Vlan_Manager::get_instance().clear_all_data();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // monitor br-tun for arp request message
  ovs_monitor_thread =
          new thread(bind(&ACA_OVS_Control::monitor,
                          &ACA_OVS_Control::get_instance(), "br-tun", "resume"));
  ovs_monitor_thread->detach();

  GoalStateV2 GoalState_builder;
  PortState new_port_states;
  SubnetState new_subnet_states;

  new_port_states.set_operation_type(OperationType::CREATE);

  // fill in port state structs for port 1
  PortConfiguration *PortConfiguration_builder = new_port_states.mutable_configuration();
  PortConfiguration_builder->set_revision_number(2);
  PortConfiguration_builder->set_update_type(UpdateType::FULL);
  PortConfiguration_builder->set_id(two_port_port_id_1);

  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  PortConfiguration_builder->set_name(two_port_port_name_1);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_1);
  PortConfiguration_builder->set_admin_state_up(true);

  PortConfiguration_FixedIp *FixedIp_builder = PortConfiguration_builder->add_fixed_ips();
  FixedIp_builder->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder->set_ip_address(two_port_vip_address_1);

  PortConfiguration_SecurityGroupId *SecurityGroup_builder =
          PortConfiguration_builder->add_security_group_ids();
  SecurityGroup_builder->set_id("2");

  auto &port_states_map = *GoalState_builder.mutable_port_states();
  port_states_map[two_port_port_id_1] = new_port_states;

  // fill in subnet state structs
  new_subnet_states.set_operation_type(OperationType::INFO);

  SubnetConfiguration *SubnetConiguration_builder =
          new_subnet_states.mutable_configuration();
  SubnetConiguration_builder->set_revision_number(2);
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_1);
  SubnetConiguration_builder->set_id(two_port_subnet_id_1);
  SubnetConiguration_builder->set_cidr("11.0.0.0/24");
  SubnetConiguration_builder->set_tunnel_id(21);

  auto &subnet_states_map = *GoalState_builder.mutable_subnet_states();
  subnet_states_map[two_port_subnet_id_1] = new_subnet_states;

  // add a new neighbor state with CREATE
  NeighborState new_neighbor_states;
  new_neighbor_states.set_operation_type(OperationType::CREATE);

  // fill in neighbor state structs for port 3
  NeighborConfiguration *NeighborConfiguration_builder =
          new_neighbor_states.mutable_configuration();
  NeighborConfiguration_builder->set_revision_number(2);

  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  NeighborConfiguration_builder->set_id(two_port_port_id_3);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_3);
  NeighborConfiguration_builder->set_host_ip_address(remote_ip_2);

  NeighborConfiguration_FixedIp *FixedIp_builder2 =
          NeighborConfiguration_builder->add_fixed_ips();
  FixedIp_builder2->set_neighbor_type(NeighborType::L2);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder2->set_ip_address(two_port_vip_address_3);

  auto &neighbor_states_map = *GoalState_builder.mutable_neighbor_states();
  neighbor_states_map[two_port_port_id_3] = new_neighbor_states;

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port 1 + port 3 neighbor in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // check to ensure the port 1 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_1 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // setup the configuration for port 2
  PortConfiguration_builder->set_id(two_port_port_id_2);
  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  PortConfiguration_builder->set_name(two_port_port_name_2);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_2);
  FixedIp_builder->set_ip_address(two_port_vip_address_2);
  FixedIp_builder->set_subnet_id(two_port_subnet_id_2);

  GoalState_builder.clear_port_states();
  port_states_map[two_port_port_id_2] = new_port_states;

  // fill in subnet state structs
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_2);
  SubnetConiguration_builder->set_id(two_port_subnet_id_2);
  SubnetConiguration_builder->set_cidr("11.0.1.0/24");
  SubnetConiguration_builder->set_tunnel_id(31);

  GoalState_builder.clear_subnet_states();
  subnet_states_map[two_port_subnet_id_2] = new_subnet_states;

  // fill in neighbor state structs for port 4
  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  NeighborConfiguration_builder->set_id(two_port_port_id_4);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_4);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_2);
  FixedIp_builder2->set_ip_address(two_port_vip_address_4);

  GoalState_builder.clear_neighbor_states();
  neighbor_states_map[two_port_port_id_4] = new_neighbor_states;

  // create a new port 2 + port 4 neighbor in demo mode
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the port 2 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_2 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  ACA_LOG_INFO("-------- After setting up, printout ifconfig: %d -------------\n", overall_rc);

  overall_rc = Aca_Net_Config::get_instance().execute_system_command("ifconfig");

  // test traffic between the two newly created ports
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_2;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_1;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test valid traffic from parent to child
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test invalid traffic from parent to child with different subnets
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // just clearing the port states and reuse the rest GoalState_builder
  new_port_states.clear_configuration();
  GoalState_builder.clear_port_states();

  // delete port 4 as L2 neighbor
  new_neighbor_states.set_operation_type(OperationType::DELETE);
  GoalState_builder.clear_neighbor_states();
  neighbor_states_map[two_port_port_id_4] = new_neighbor_states;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  ACA_LOG_INFO("-------- Delete port 4 was succcessful: %d -------------\n", overall_rc);

  ACA_LOG_INFO("-------- After deleting port 4, printout ifconfig: %d -------------\n",
               overall_rc);

  overall_rc = Aca_Net_Config::get_instance().execute_system_command("ifconfig");

  overall_rc = EXIT_SUCCESS;

  // should not be able to ping port 4 anymore
  cmd_string = "ping -I " + two_port_vip_address_2 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_NE(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // but still have to ping port 3 since it is not deleted
  cmd_string = "ping -I " + two_port_vip_address_1 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // clean up

  // free the allocated configurations since we are done with it now
  new_neighbor_states.clear_configuration();
  new_subnet_states.clear_configuration();
}

TEST(ovs_l2_test_cases, DISABLED_2_ports_CREATE_test_traffic_CHILD_V2)
{
  string two_port_vmac_address_1 = "fa:16:3e:d7:f2:6a";
  string two_port_vmac_address_2 = "fa:16:3e:d7:f2:6b";
  string two_port_vmac_address_3 = "fa:16:3e:d7:f2:6c";
  string two_port_vmac_address_4 = "fa:16:3e:d7:f2:6d";

  string two_port_vip_address_1 = "11.0.0.105";
  string two_port_vip_address_2 = "11.0.1.106";
  string two_port_vip_address_3 = "11.0.0.107";
  string two_port_vip_address_4 = "11.0.1.108";

  string two_port_vpc_id_1 = "2b08a5bc-b718-11ea-b3de-111111111112";
  string two_port_vpc_id_2 = "2b08a5bc-b718-11ea-b3de-222222222223";

  string two_port_port_id_1 = "11111111-b718-11ea-b3de-111111111112";
  string two_port_port_id_2 = "12222222-b718-11ea-b3de-111111111113";
  string two_port_port_id_3 = "13333333-b718-11ea-b3de-111111111114";
  string two_port_port_id_4 = "14444444-b718-11ea-b3de-111111111115";

  string two_port_port_name_1 = aca_get_port_name(two_port_port_id_1);
  string two_port_port_name_2 = aca_get_port_name(two_port_port_id_2);
  string two_port_port_name_3 = aca_get_port_name(two_port_port_id_3);
  string two_port_port_name_4 = aca_get_port_name(two_port_port_id_4);

  string two_port_subnet_id_1 = "27330ae4-b718-11ea-b3df-111111111113";
  string two_port_subnet_id_2 = "27330ae4-b718-11ea-b3df-222222222224";

  ulong not_care_culminative_time = 0;
  string cmd_string;
  int overall_rc;

  ACA_Vlan_Manager::get_instance().clear_all_data();

  // delete br-int and br-tun bridges
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-int", not_care_culminative_time, overall_rc);

  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "del-br br-tun", not_care_culminative_time, overall_rc);

  // create and setup br-int and br-tun bridges, and their patch ports
  overall_rc = ACA_OVS_L2_Programmer::get_instance().setup_ovs_bridges_if_need();
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // monitor br-tun for arp request message
  ovs_monitor_thread =
          new thread(bind(&ACA_OVS_Control::monitor,
                          &ACA_OVS_Control::get_instance(), "br-tun", "resume"));

  GoalStateV2 GoalState_builder;
  PortState new_port_states;
  SubnetState new_subnet_states;

  new_port_states.set_operation_type(OperationType::CREATE);

  // fill in port state structs for port 3
  PortConfiguration *PortConfiguration_builder = new_port_states.mutable_configuration();
  PortConfiguration_builder->set_revision_number(2);
  PortConfiguration_builder->set_update_type(UpdateType::FULL);
  PortConfiguration_builder->set_id(two_port_port_id_3);

  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  PortConfiguration_builder->set_name(two_port_port_name_3);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_3);
  PortConfiguration_builder->set_admin_state_up(true);

  PortConfiguration_FixedIp *FixedIp_builder = PortConfiguration_builder->add_fixed_ips();
  FixedIp_builder->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder->set_ip_address(two_port_vip_address_3);

  PortConfiguration_SecurityGroupId *SecurityGroup_builder =
          PortConfiguration_builder->add_security_group_ids();
  SecurityGroup_builder->set_id("2");

  auto &port_states_map = *GoalState_builder.mutable_port_states();
  port_states_map[two_port_port_id_3] = new_port_states;

  // fill in subnet state structs
  new_subnet_states.set_operation_type(OperationType::INFO);

  SubnetConfiguration *SubnetConiguration_builder =
          new_subnet_states.mutable_configuration();
  SubnetConiguration_builder->set_revision_number(2);
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_1);
  SubnetConiguration_builder->set_id(two_port_subnet_id_1);
  SubnetConiguration_builder->set_cidr("11.0.0.0/24");
  SubnetConiguration_builder->set_tunnel_id(21);

  auto &subnet_states_map = *GoalState_builder.mutable_subnet_states();
  subnet_states_map[two_port_subnet_id_1] = new_subnet_states;

  // add a new neighbor state with CREATE
  NeighborState new_neighbor_states;
  new_neighbor_states.set_operation_type(OperationType::CREATE);

  // fill in neighbor state structs for port 1
  NeighborConfiguration *NeighborConfiguration_builder =
          new_neighbor_states.mutable_configuration();
  NeighborConfiguration_builder->set_revision_number(2);

  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_1);
  NeighborConfiguration_builder->set_id(two_port_port_id_1);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_1);
  NeighborConfiguration_builder->set_host_ip_address(remote_ip_1);

  NeighborConfiguration_FixedIp *FixedIp_builder2 =
          NeighborConfiguration_builder->add_fixed_ips();
  FixedIp_builder2->set_neighbor_type(NeighborType::L2);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_1);
  FixedIp_builder2->set_ip_address(two_port_vip_address_1);

  auto &neighbor_states_map = *GoalState_builder.mutable_neighbor_states();
  neighbor_states_map[two_port_port_id_1] = new_neighbor_states;

  // set demo mode
  bool previous_demo_mode = g_demo_mode;
  g_demo_mode = true;

  // create a new port 3 + port 1 neighbor in demo mode
  GoalStateOperationReply gsOperationalReply;

  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // check to ensure the port 3 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_3 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // setup the configuration for port 4
  PortConfiguration_builder->set_id(two_port_port_id_4);
  PortConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  PortConfiguration_builder->set_name(two_port_port_name_4);
  PortConfiguration_builder->set_mac_address(two_port_vmac_address_4);
  FixedIp_builder->set_ip_address(two_port_vip_address_4);
  FixedIp_builder->set_subnet_id(two_port_subnet_id_2);

  GoalState_builder.clear_port_states();
  port_states_map[two_port_port_id_4] = new_port_states;

  // fill in subnet state structs
  SubnetConiguration_builder->set_vpc_id(two_port_vpc_id_2);
  SubnetConiguration_builder->set_id(two_port_subnet_id_2);
  SubnetConiguration_builder->set_cidr("11.0.1.0/24");
  SubnetConiguration_builder->set_tunnel_id(31);

  GoalState_builder.clear_subnet_states();
  subnet_states_map[two_port_subnet_id_2] = new_subnet_states;

  // fill in port state structs for NEIGHBOR_CREATE_UPDATE for port 2
  NeighborConfiguration_builder->set_vpc_id(two_port_vpc_id_2);
  NeighborConfiguration_builder->set_id(two_port_port_id_2);
  NeighborConfiguration_builder->set_mac_address(two_port_vmac_address_2);
  FixedIp_builder2->set_subnet_id(two_port_subnet_id_2);
  FixedIp_builder2->set_ip_address(two_port_vip_address_2);

  GoalState_builder.clear_neighbor_states();
  neighbor_states_map[two_port_port_id_2] = new_neighbor_states;

  // create a new port 4 + port 2 neighbor in demo mode
  overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
  ASSERT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // restore demo mode
  g_demo_mode = previous_demo_mode;

  // check to ensure the port 4 is created and setup correctly
  ACA_OVS_L2_Programmer::get_instance().execute_ovsdb_command(
          "get Interface " + two_port_port_name_4 + " ofport",
          not_care_culminative_time, overall_rc);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // test traffic between the two newly created ports
  cmd_string = "ping -I " + two_port_vip_address_3 + " -c1 " + two_port_vip_address_4;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  cmd_string = "ping -I " + two_port_vip_address_4 + " -c1 " + two_port_vip_address_3;
  overall_rc = Aca_Net_Config::get_instance().execute_system_command(cmd_string);
  EXPECT_EQ(overall_rc, EXIT_SUCCESS);
  overall_rc = EXIT_SUCCESS;

  // clean up

  // free the allocated configurations since we are done with it now
  new_port_states.clear_configuration();
  new_neighbor_states.clear_configuration();
  new_subnet_states.clear_configuration();

  // wait for parent to ping child
  ovs_monitor_thread->join();

  // not deleting br-int and br-tun bridges so that parent can ping the two new ports
}
