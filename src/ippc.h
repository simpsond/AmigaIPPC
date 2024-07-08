//
// Created by Dustin Simpson on 7/6/24.
//

#ifndef AMIGAIPCDEMO_SRC_IPPC_H_
#define AMIGAIPCDEMO_SRC_IPPC_H_

#include <proto/dos.h>
#include <exec/ports.h>

struct IPPCRequest {
  struct MsgPort* response_port;
  STRPTR command_name;
  ULONG payload_sz;
  void* payload;
};

struct IPPCResponse {
  USHORT chunk_id;
  ULONG length;
  void* data;
};

struct IPPCRequestMsg {
  struct Message msg;
  struct IPPCRequest request;
};

struct CommandResponse {
  struct Message msg;
  struct IPPCResponse* response;
};

void CreateCommandMessage(struct IPPCRequestMsg* msg, STRPTR command, void* data, ULONG sz);
void CallTaskRPC(struct Task* task, struct IPPCRequestMsg* cmd, void(*cb)(struct CommandResponse* data));

void RPCGetCommand(struct MsgPort* port, void(*OnCommand)(struct IPPCRequest*, void(*CB)(struct IPPCRequest* request, struct IPPCResponse* response)));

void CopyRPCRequest(struct IPPCRequest* src, struct IPPCRequest* dst);
void FreeRPCRequest(struct IPPCRequest* request);

void FreeRPCResponse(struct IPPCResponse* response);

#endif //AMIGAIPCDEMO_SRC_IPPC_H_
