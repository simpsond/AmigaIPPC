/**
MIT License

Copyright (c) 2024 Dustin Simpson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include "ippc.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>

#include <string.h>

struct IPPCRequest* CreateIPPCRequest( STRPTR command, void* data, ULONG sz) {
  struct IPPCRequest* request;

  ULONG command_name_sz;
  command_name_sz = (ULONG) strlen((const char*) command) + 1;

  request = AllocMem(sizeof(struct IPPCRequest), MEMF_ANY | MEMF_CLEAR);

  request->command_name = AllocMem(command_name_sz, MEMF_ANY | MEMF_CLEAR);
  CopyMem(command, request->command_name, command_name_sz);

  request->payload_sz = sz;
  request->payload = AllocMem(sz, MEMF_ANY | MEMF_CLEAR);
  CopyMem(data, request->payload, sz);

  request->response_port = CreateMsgPort();

  return request;
}


/**
 * @brief Essentially a convenience function to set the command name and data on an already allocated RequestMessage
 * @param msg space needs to be allocated before passing into this function
 * @param command name of command, assumed this pointer will be valid through execution
 * @param data assumed this pointer will be valid through execution
 */
void CreateCommandMessage(struct RequestMessage* msg, STRPTR command, void* data, ULONG sz) {
  ULONG command_name_sz;
  command_name_sz = (ULONG) strlen((const char*) command) + 1;

  msg->request = AllocMem(sizeof(struct IPPCRequest), MEMF_ANY | MEMF_CLEAR);

  msg->request->command_name = AllocMem(command_name_sz, MEMF_ANY | MEMF_CLEAR);
  CopyMem(command, msg->request->command_name, command_name_sz);

  msg->request->payload_sz = sz;
  msg->request->payload = AllocMem(sz, MEMF_ANY | MEMF_CLEAR);
  CopyMem(data, msg->request->payload, sz);
}

void CallTaskRPC(struct Process* proc, struct IPPCRequest* cmd, void(*cb)(struct CommandResponse* data)) {
  CallPortRPC(&proc->pr_MsgPort, cmd, cb);
}

void CallPortRPC(struct MsgPort* port, struct IPPCRequest* cmd, void(*cb)(struct CommandResponse* data)) {
  struct RequestMessage message;
  ULONG packet_len;

  message.msg.mn_ReplyPort = CreateMsgPort();
  message.msg.mn_Length = sizeof(struct RequestMessage);
  message.request = cmd;

  PutMsg(port, (struct Message*) &message);
  WaitPort(message.msg.mn_ReplyPort);
  GetMsg(message.msg.mn_ReplyPort);
  DeleteMsgPort(message.msg.mn_ReplyPort);

  do {
    struct CommandResponse* cmd_response;

    WaitPort(cmd->response_port);
    cmd_response = (struct CommandResponse *) GetMsg(cmd->response_port);
    packet_len = cmd_response->response->length;

    cb(cmd_response);
    ReplyMsg((struct Message *) cmd_response);
  } while(packet_len > 0);
}

void OnCommandCB(struct IPPCRequest* request, struct IPPCResponse* response) {
  struct CommandResponse response_message;
  #ifdef ENABLE_KPRINT
    KPrintF("In OnCommandCB\n");
  #endif

  response_message.msg.mn_ReplyPort = CreateMsgPort();
  response_message.msg.mn_Length = sizeof(struct CommandResponse);
  response_message.response = response;
  PutMsg(request->response_port, (struct Message*)&response_message);
  WaitPort(response_message.msg.mn_ReplyPort);
  GetMsg(response_message.msg.mn_ReplyPort);
  DeleteMsgPort(response_message.msg.mn_ReplyPort);
}

void RPCGetCommand(struct MsgPort* port, void(*OnCommand)(struct IPPCRequest*, void(*CB)(struct IPPCRequest* request, struct IPPCResponse* response))) {
  struct RequestMessage* message;

  if((message = (struct RequestMessage*)GetMsg(port))) {
    struct IPPCResponse response_terminator = {0, 0, NULL};
    #ifdef ENABLE_KPRINT
    KPrintF("Got a message\n");
    #endif

    ReplyMsg((struct Message*)message);
    OnCommand(message->request, OnCommandCB);
    OnCommandCB(message->request, &response_terminator);
  }
}

void FreeIPPCRequest(struct IPPCRequest* request) {
  ULONG command_name_sz;
  command_name_sz = (ULONG) strlen((const char*) request->command_name) + 1;

  #ifdef ENABLE_KPRINT
  KPrintF("in FreeIPPCRequest\n");
  #endif

//  Forbid(); // We can't risk a getting another message while trying to delete the message port
//  while(GetMsg(request->response_port)) { // Throw away any messages still waiting
//    #ifdef ENABLE_KPRINT
//    KPrintF("We had another message!!!!\n");
//    #endif
//  }

#ifdef ENABLE_KPRINT
    KPrintF("going to DeleteMsgPort\n");
#endif
    DeleteMsgPort(request->response_port);
//  Permit();

  if(request->payload && request->payload_sz > 0) {
    #ifdef ENABLE_KPRINT
    KPrintF("going to FreeMem on payload\n");
    #endif
    FreeMem(request->payload, request->payload_sz);
    request->payload_sz = 0;
  } else {
    #ifdef ENABLE_KPRINT
    KPrintF("Payload empty, will not FreeMem\n");
    #endif

  }
  if(request->command_name && command_name_sz > 0) {
    #ifdef ENABLE_KPRINT
    KPrintF("going to FreeMem on command_name, size: %ld\n", command_name_sz);
    #endif
    FreeMem(request->command_name, command_name_sz);
  }
}

