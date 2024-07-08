//
// Created by Dustin Simpson on 7/6/24.
//

#include "ippc.h"

#include <proto/exec.h>
#include <string.h>

/**
 * @brief Essentially a convenience function to set the command name and data on an already allocated IPPCRequestMsg
 * @param msg space needs to be allocated before passing into this function
 * @param command name of command, assumed this pointer will be valid through execution
 * @param data assumed this pointer will be valid through execution
 */
void CreateCommandMessage(struct IPPCRequestMsg* msg, STRPTR command, void* data, ULONG sz) {
  msg->request.command_name = command;
  msg->request.payload = data;
  msg->request.payload_sz = sz;
}

void CallTaskRPC(struct Task* task, struct IPPCRequestMsg* cmd, void(*cb)(struct CommandResponse* data)) {
  struct MsgPort* response_port, *cmd_response_port;
  struct CommandResponse* cmd_response;
  ULONG packet_len;

  cmd->msg.mn_ReplyPort = CreateMsgPort();
  cmd->msg.mn_Length = sizeof(struct IPPCRequestMsg);
  cmd_response_port = CreateMsgPort();
  cmd->request.response_port = cmd_response_port;

  PutMsg(&((struct Process*)task)->pr_MsgPort, cmd);
  WaitPort(cmd->msg.mn_ReplyPort);
  GetMsg(cmd->msg.mn_ReplyPort);
  DeleteMsgPort(cmd->msg.mn_ReplyPort);

  GetChunk:
  WaitPort(cmd_response_port);
  cmd_response = (struct CommandResponse*)GetMsg(cmd_response_port);
  packet_len = cmd_response->response->length;

  if(packet_len > 0) {
    cb(cmd_response);
    ReplyMsg(cmd_response);
    goto GetChunk;
  } else {
    ReplyMsg(cmd_response);
  }
  DeleteMsgPort(cmd_response_port);
}

void OnCommandCB(struct IPPCRequest* request, struct IPPCResponse* response) {
  struct CommandResponse response_message;

  response_message.msg.mn_ReplyPort = CreateMsgPort();
  response_message.msg.mn_Length = sizeof(struct CommandResponse);
  response_message.response = response;
  PutMsg(request->response_port, &response_message);
  WaitPort(response_message.msg.mn_ReplyPort);
  GetMsg(response_message.msg.mn_ReplyPort);
  DeleteMsgPort(response_message.msg.mn_ReplyPort);
}

void RPCGetCommand(struct MsgPort* port, void(*OnCommand)(struct IPPCRequest*, void(*CB)(struct IPPCRequest* request, struct IPPCResponse* response))) {
  struct IPPCRequestMsg* message;
//  struct CommandResponse response_message;
//  struct MsgPort* cmd_response_port;
//  struct IPPCResponse* response;
  struct IPPCRequest request;

  if((message = (struct IPPCRequestMsg*)GetMsg(port))) {
    CopyRPCRequest(&(message->request), &request);

//    cmd_response_port = message->request.response_port;
    ReplyMsg(message);
    OnCommand(&request, OnCommandCB);

//    CmdGetRandom(cmd_response_port, *(USHORT*)message->request.payload);
    FreeRPCRequest(&request);
  }

}

void CopyRPCRequest(struct IPPCRequest* src, struct IPPCRequest* dst) {
  ULONG command_name_sz;

  command_name_sz = (ULONG) strlen((const char*) src->command_name) + 1;
  dst->payload_sz = src->payload_sz;
  dst->response_port = src->response_port;
  dst->payload = AllocMem(src->payload_sz, MEMF_ANY | MEMF_CLEAR);
  CopyMem(src->payload, dst->payload, src->payload_sz);
  dst->command_name = AllocMem(command_name_sz, MEMF_ANY | MEMF_CLEAR);
  CopyMem(src->command_name, dst->command_name, command_name_sz);
}

void FreeRPCRequest(struct IPPCRequest* request) {
  ULONG command_name_sz;

  command_name_sz = (ULONG) strlen((const char*) request->command_name) + 1;

  FreeMem(request->payload, request->payload_sz);
  request->payload_sz = 0;
  FreeMem(request->command_name, command_name_sz);
}

void FreeRPCResponse(struct IPPCResponse* response) {
  FreeMem(response->data, response->length);
  response->length = 0;
}
