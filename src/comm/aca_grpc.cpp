/*
 *
 * Copyright 2015 gRPC authors.
 * Copyright 2019 The Alcor Authors - file modified.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "goalstateprovisioner.grpc.pb.h"
#include "aca_comm_mgr.h"
#include "aca_log.h"
#include "aca_grpc.h"

extern string g_grpc_server_port;
// extern string g_ncm_address;
// extern string g_ncm_port;

using namespace alcor::schema;
using aca_comm_manager::Aca_Comm_Manager;

// void GoalStateProvisionerImpl::RequestGoalStates(HostRequest *request,
//                                                  grpc::CompletionQueue *cq)
// {
//   grpc::ClientContext ctx;
//   alcor::schema::HostRequestReply reply;

//   // check current grpc channel state, try to connect if needed
//   grpc_connectivity_state current_state = chan_->GetState(true);
//   if (current_state == grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN ||
//       current_state == grpc_connectivity_state::GRPC_CHANNEL_TRANSIENT_FAILURE) {
//     ACA_LOG_INFO("%s, it is: [%d]\n",
//                  "Channel state is not READY/CONNECTING/IDLE. Try to reconnnect.",
//                  current_state);
//     this->ConnectToNCM();
//     reply.mutable_operation_statuses()->Add();
//     reply.mutable_operation_statuses()->at(0).set_operation_status(OperationStatus::FAILURE);
//     return;
//   }
//   AsyncClientCall *call = new AsyncClientCall;
//   call->response_reader = stub_->AsyncRequestGoalStates(&call->context, *request, cq);
//   call->response_reader->Finish(&call->reply, &call->status, (void *)call);
//   return;
// }

Status
GoalStateProvisionerImpl::PushNetworkResourceStates(ServerContext * /* context */,
                                                    const GoalState *goalState,
                                                    GoalStateOperationReply *goalStateOperationReply)
{
  GoalState received_goal_state = *goalState;

  int rc = Aca_Comm_Manager::get_instance().update_goal_state(
          received_goal_state, *goalStateOperationReply);
  if (rc == EXIT_SUCCESS) {
    ACA_LOG_INFO("Control Fast Path synchronized - Successfully updated host with latest goal state %d.\n",
                 rc);
  } else if (rc == EINPROGRESS) {
    ACA_LOG_INFO("Control Fast Path synchronized - Update host with latest goal state returned pending, rc=%d.\n",
                 rc);
  } else {
    ACA_LOG_ERROR("Control Fast Path synchronized - Failed to update host with latest goal state, rc=%d.\n",
                  rc);
  }

  return Status::OK;
}

Status GoalStateProvisionerImpl::PushGoalStatesStream(
        ServerContext * /* context */,
        ServerReaderWriter<GoalStateOperationReply, GoalStateV2> *stream)
{
  GoalStateV2 goalStateV2;
  GoalStateOperationReply gsOperationReply;
  int rc = EXIT_SUCCESS; //EXIT_FAILURE;
  ACA_LOG_INFO("%s\n", "[PushGoalStatesStream] Begins");
  std::chrono::_V2::steady_clock::time_point receives_goalstate_time_previous =
          std::chrono::steady_clock::now();
  while (stream->Read(&goalStateV2)) {
    // Use a separate thread to update the goalstateV2
    // ACA_LOG_INFO("[PushGoalStatesStream] Receives an GS, new a thread to process it, thread id: [%ld]\n",
    //              std::this_thread::get_id());
    std::chrono::_V2::steady_clock::time_point receives_goalstate_time_current =
            std::chrono::steady_clock::now();
    // std::thread(std::bind(&GoalStateProvisionerImpl::UpdateGoalStateInNewThread,
    //                       this, stream, goalStateV2, gsOperationReply))
    //         .detach();
    auto goalstate_receive_interval =
            std::chrono::duration_cast<std::chrono::microseconds>(
                    receives_goalstate_time_current - receives_goalstate_time_previous)
                    .count();

    ACA_LOG_INFO("[METRICS] Elapsed time between receiving the last and current goalstate took: %ld microseconds or %ld milliseconds\n",
                 goalstate_receive_interval, (goalstate_receive_interval / 1000));
    receives_goalstate_time_previous = receives_goalstate_time_current;
    std::chrono::_V2::steady_clock::time_point start = std::chrono::steady_clock::now();

    if (goalStateV2.neighbor_states_size() == 1) {
      // if there's only one neighbor state, it means that it is pushed
      // because of the on-demand request
      auto received_gs_time_high_res = std::chrono::high_resolution_clock::now();
      auto neighbor_id = goalStateV2.neighbor_states().begin()->first.c_str();
      ACA_LOG_INFO("Neighbor ID: %s received at: %ld milliseconds\n", neighbor_id,
                   std::chrono::duration_cast<std::chrono::milliseconds>(
                           received_gs_time_high_res.time_since_epoch())
                           .count());
    }
    // rc = Aca_Comm_Manager::get_instance().update_goal_state(goalStateV2, gsOperationReply);
    if (rc == EXIT_SUCCESS) {
      ACA_LOG_INFO("Control Fast Path streaming - Successfully updated host with latest goal state %d.\n",
                   rc);
    } else if (rc == EINPROGRESS) {
      ACA_LOG_INFO("Control Fast Path streaming - Update host with latest goal state returned pending, rc=%d.\n",
                   rc);
    } else {
      ACA_LOG_ERROR("Control Fast Path streaming - Failed to update host with latest goal state, rc=%d.\n",
                    rc);
    }
    std::chrono::_V2::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto message_total_operation_time =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    ACA_LOG_INFO("[METRICS] Received goalstate at: [%ld], update finished at: [%ld]\nElapsed time for update goalstate operation took: %ld microseconds or %ld milliseconds\n",
                 start, end, message_total_operation_time,
                 (message_total_operation_time / 1000));
    stream->Write(gsOperationReply);
    gsOperationReply.Clear();
  }

  return Status::OK;
}

// void GoalStateProvisionerImpl::UpdateGoalStateInNewThread(
//         ServerReaderWriter<GoalStateOperationReply, GoalStateV2> *stream,
//         GoalStateV2 goalStateV2, GoalStateOperationReply gsOperationReply)
// {
//   ACA_LOG_INFO("[UpdateGoalStateInNewThread] Begins to update goalstate with Thread_ID: [%ld]\n",
//                std::this_thread::get_id());

//   int rc = EXIT_FAILURE;
//   std::chrono::_V2::steady_clock::time_point start = std::chrono::steady_clock::now();

//   if (goalStateV2.neighbor_states_size() == 1) {
//     // if there's only one neighbor state, it means that it is pushed
//     // because of the on-demand request
//     auto received_gs_time_high_res = std::chrono::high_resolution_clock::now();
//     auto neighbor_id = goalStateV2.neighbor_states().begin()->first.c_str();
//     ACA_LOG_INFO("Neighbor ID: %s received at: %ld milliseconds\n", neighbor_id,
//                  std::chrono::duration_cast<std::chrono::milliseconds>(
//                          received_gs_time_high_res.time_since_epoch())
//                          .count());
//   }
//   rc = Aca_Comm_Manager::get_instance().update_goal_state(goalStateV2, gsOperationReply);
//   if (rc == EXIT_SUCCESS) {
//     ACA_LOG_INFO("Control Fast Path streaming - Successfully updated host with latest goal state %d.\n",
//                  rc);
//   } else if (rc == EINPROGRESS) {
//     ACA_LOG_INFO("Control Fast Path streaming - Update host with latest goal state returned pending, rc=%d.\n",
//                  rc);
//   } else {
//     ACA_LOG_ERROR("Control Fast Path streaming - Failed to update host with latest goal state, rc=%d.\n",
//                   rc);
//   }
//   std::chrono::_V2::steady_clock::time_point end = std::chrono::steady_clock::now();
//   auto message_total_operation_time =
//           std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

//   ACA_LOG_INFO("[METRICS] Received goalstate at: [%ld], update finished at: [%ld]\nElapsed time for update goalstate operation took: %ld microseconds or %ld milliseconds\n",
//                start, end, message_total_operation_time,
//                (message_total_operation_time / 1000));
//   stream->Write(gsOperationReply);
//   gsOperationReply.Clear();
//   ACA_LOG_INFO("%s\n", "[UpdateGoalStateInNewThread] Finished update goalstate");

//   return;
// }

Status GoalStateProvisionerImpl::ShutDownServer()
{
  ACA_LOG_INFO("%s", "Shutdown server");
  server->Shutdown();
  return Status::OK;
}

// void GoalStateProvisionerImpl::ConnectToNCM()
// {
//   ACA_LOG_INFO("%s\n", "Trying to init a new sub to connect to the NCM");
//   grpc::ChannelArguments args;
//   // Channel does a keep alive ping every 10 seconds;
//   args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
//   // If the channel does receive the keep alive ping result in 20 seconds, it closes the connection
//   args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 20000);

//   // args.SetInt(GRPC_ARG_MAX_CONCURRENT_STREAMS, 500);

//   // Allow keep alive ping even if there are no calls in flight
//   args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

//   chan_ = grpc::CreateCustomChannel(g_ncm_address + ":" + g_ncm_port,
//                                     grpc::InsecureChannelCredentials(), args);
//   stub_ = GoalStateProvisioner::NewStub(chan_);

//   ACA_LOG_INFO("%s\n", "After initing a new sub to connect to the NCM");
// }

void GoalStateProvisionerImpl::RunServer()
{
  // this->ConnectToNCM();
  ServerBuilder builder;
  // Try to unlimit the size of goalstate received.
  builder.SetMaxMessageSize(INT_MAX);
  builder.SetMaxReceiveMessageSize(INT_MAX);
  string GRPC_SERVER_ADDRESS = "0.0.0.0:" + g_grpc_server_port;
  // builder.AddChannelArgument(GRPC_ARG_MAX_CONCURRENT_STREAMS, 500);
  // grpc::ResourceQuota rq;
  // rq.SetMaxThreads(32);
  // builder.SetResourceQuota(rq);
  // builder.SetSyncServerOption(grpc_impl::ServerBuilder::SyncServerOption::NUM_CQS, 500);
  // builder.SetSyncServerOption(grpc_impl::ServerBuilder::SyncServerOption::MAX_POLLERS, 500);
  // builder.SetSyncServerOption(grpc_impl::ServerBuilder::SyncServerOption::MIN_POLLERS, 200);
  builder.AddListeningPort(GRPC_SERVER_ADDRESS, grpc::InsecureServerCredentials());
  builder.RegisterService(this);
  server = builder.BuildAndStart();
  ACA_LOG_INFO("Streaming capable GRPC server listening on %s\n",
               GRPC_SERVER_ADDRESS.c_str());
  server->Wait();
}