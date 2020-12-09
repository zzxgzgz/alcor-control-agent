// Copyright 2019 The Alcor Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gtest/gtest.h"
#define private public
#include "aca_util.h"
#include "goalstateprovisioner.grpc.pb.h"
#include <string.h>
#include "aca_comm_mgr.h"
#include "aca_zeta_programming.h"
#include <fstream>
#include <nlohmann/json.hpp>//parse json file
#include<iostream>

using namespace std;
using namespace aca_comm_manager;
using namespace alcor::schema;
using namespace aca_zeta_programming;

extern string vmac_address_1;
extern string vmac_address_2;
extern string vmac_address_3;
extern string vmac_address_4;
extern string vip_address_1;
extern string vip_address_2;
extern string vip_address_3;
extern string vip_address_4;
extern string remote_ip_1;
extern string remote_ip_2;

extern string node_mac_address_1;
extern string node_mac_address_2;
extern string node_mac_address_3;
extern string node_mac_address_4;

extern string vpc_id_1;
extern string vpc_id_2;

string auxGateway_id_1 = "11";
string auxGateway_id_2 = "22";

uint tunnel_id_1 = 555;
uint tunnel_id_2 = 666;
uint oam_port_1 = 6799;
uint oam_port_2 = 6800;

void aca_test_zeta_setup(zeta_gateway_path_config_file)
{
    ifstream ifs(zeta_gateway_path_config_file);
    if(!ifs)
　　    cout<<zeta_gateway_path_config_file<<"open error"<<endl;
    Value root;
    Reader reader;
    // TODO: Read the configuration file
    if(reader.parse(ifs,root))
    {
        cout<<root.toStyledString()<<endl;
        Value &arr = root[""][""];
        //...
    }
    // TODO: construct GoalState,push to aca
    GoalState GoalState_builder;
    //...
    overall_rc = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);
    ASSERT_EQ(overall_rc, EXIT_SUCCESS);
}

TEST(zeta_programming_test_cases, create_or_update_zeta_config_valid)
{
  int retcode = 0;

  // fill in auxgateway state structs
  AuxGateway new_auxGateway;
  new_auxGateway.set_id(auxGateway_id_1);
  AuxGateway_zeta *zeta_info = new_auxGateway.mutable_zeta_info();
  zeta_info->set_port_inband_operation(oam_port_1);
  AuxGateway_destination *destinaton;

  destinaton = new_auxGateway.add_destinations();
  destinaton->set_ip_address(remote_ip_1);
  destinaton->set_mac_address(node_mac_address_3);

  destinaton = new_auxGateway.add_destinations();
  destinaton->set_ip_address(remote_ip_2);
  destinaton->set_mac_address(node_mac_address_4);

  retcode = ACA_Zeta_Programming::get_instance().create_or_update_zeta_config(
          new_auxGateway, vpc_id_2, tunnel_id_2);

  EXPECT_EQ(retcode, EXIT_SUCCESS);
}

TEST(zeta_programming_test_cases, delete_zeta_config_valid)
{
  int retcode = 0;

  // fill in auxgateway state structs
  AuxGateway new_auxGateway;
  new_auxGateway.set_id(auxGateway_id_1);
  AuxGateway_zeta *zeta_info = new_auxGateway.mutable_zeta_info();
  zeta_info->set_port_inband_operation(oam_port_1);
  AuxGateway_destination *destinaton;

  destinaton = new_auxGateway.add_destinations();
  destinaton->set_ip_address(remote_ip_1);
  destinaton->set_mac_address(node_mac_address_3);

  destinaton = new_auxGateway.add_destinations();
  destinaton->set_ip_address(remote_ip_2);
  destinaton->set_mac_address(node_mac_address_4);

  retcode = ACA_Zeta_Programming::get_instance().delete_zeta_config(
          new_auxGateway, vpc_id_2, tunnel_id_2);

  EXPECT_EQ(retcode, EXIT_SUCCESS);
}

TEST(zeta_programming_test_cases, DISABLED_auxgateway_test)
{
  int retcode;
  GoalState GoalState_builder;
  VpcState *new_vpc_states = GoalState_builder.add_vpc_states();
  PortState *new_port_states = GoalState_builder.add_port_states();

  // fill in vpc state structs
  VpcConfiguration *VpcConfiguration_builder = new_vpc_states->mutable_configuration();
  VpcConfiguration_builder->set_tunnel_id(tunnel_id_1);
  VpcConfiguration_builder->set_id(vpc_id_1);

  // fill in auxgateway state structs
  AuxGateway *auxGateway = VpcConfiguration_builder->mutable_auxiliary_gateway();
  auxGateway->set_id(auxGateway_id_2);

  AuxGateway_zeta *zeta_info = auxGateway->mutable_zeta_info();
  zeta_info->set_port_inband_operation(oam_port_2);

  AuxGateway_destination *destinaton;

  destinaton = auxGateway->add_destinations();
  destinaton->set_ip_address(remote_ip_1);
  destinaton->set_mac_address(node_mac_address_1);

  destinaton = auxGateway->add_destinations();
  destinaton->set_ip_address(remote_ip_2);
  destinaton->set_mac_address(node_mac_address_2);

  // fill in port state structs
  PortConfiguration *PortConfiguration_builder = new_port_states->mutable_configuration();
  PortConfiguration_builder->set_vpc_id(vpc_id_1);
  new_port_states->set_operation_type(OperationType::CREATE);

  GoalStateOperationReply gsOperationalReply;

  retcode = Aca_Comm_Manager::get_instance().update_goal_state(
          GoalState_builder, gsOperationalReply);

  EXPECT_EQ(retcode, EXIT_SUCCESS);
}

TEST(zeta_programming_test_cases, DISABLED_zeta_gateway_path_CHILD)
{
    // TODO: The relative path of the CHILD configuration file
    string zeta_gateway_path_CHILD_config_file="./...";
    aca_test_zeta_setup(zeta_gateway_path_CHILD_config_file);
}

TEST(zeta_programming_test_cases, DISABLED_zeta_gateway_path_PARENT)
{
    // TODO: The relative path of the PARENT configuration file
    string zeta_gateway_path_CHILD_config_file="./...";
    aca_test_zeta_setup(zeta_gateway_path_PARENT_config_file);
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