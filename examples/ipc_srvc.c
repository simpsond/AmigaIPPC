//
// Created by Dustin Simpson on 7/4/24.
//
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/debug_protos.h>
#include <string.h>

#include "ippc.h"

struct ExecBase* SysBase;
struct DosLibrary* DOSBase;

#ifdef __GNUC__
#define __saveds
asm("bra _entry");
#endif

//void CmdGetRandom(struct MsgPort* port, int count);
void OnCommand(struct IPPCRequest* request, void(*cb)(struct IPPCRequest* request, struct IPPCResponse* response));

void entry() {
  struct IPPCRequestMsg* message;
  struct Process* self_process;
  struct MsgPort* cmd_response_port;

  SysBase = *(struct ExecBase**)4;
  DOSBase = (struct DosLibrary*)OpenLibrary("dos.library", 0);

  self_process = (struct Process*)FindTask(NULL);

  for(;;) {
    KPrintF("ipc_srvc ping\n");
    RPCGetCommand(&self_process->pr_MsgPort, OnCommand);
//    message = (struct IPPCRequestMsg*)GetMsg(&self_process->pr_MsgPort);
//    if(message) {
//      KPrintF("got a message!\n");
//      cmd_response_port = message->response_port;
//      ReplyMsg(message);
//      CmdGetRandom(cmd_response_port, *(USHORT*)message->data);
//    }
    Delay(100);
  }
}


void OnCommand(struct IPPCRequest* request, void(*cb)(struct IPPCRequest* request, struct IPPCResponse* response)) {
  if(strcmp("get_random", (const char*)request->command_name) == 0) {
    struct IPPCResponse response;
    response.length = sizeof(ULONG);
    response.data = AllocMem(response.length, MEMF_ANY | MEMF_CLEAR);
    KPrintF("in get_random, %l requested\n", *(USHORT *)request->payload);
    USHORT how_many = *(USHORT *)request->payload;
    for(int i = 0; i < how_many; i++) {
      *(ULONG*)response.data = 42;
      response.chunk_id = i+1;
      cb(request, &response);
    }
    FreeMem(response.data, response.length);
    response.length = 0;
    response.chunk_id++;
    cb(request, &response);
  }
}

//void CmdGetRandom(struct MsgPort* port, int count) {
//  struct CommandResponse response;
//  response.response.chunk_id = 0;
//  response.msg.mn_ReplyPort = CreateMsgPort();
//  response.msg.mn_Length = sizeof(struct CommandResponse);
//
//  for(int i = 0; i < count; i++) {
//    response.response.data = AllocVec(sizeof(ULONG), MEMF_ANY);
//    *((ULONG *) response.response.data) = 42;
//    KPrintF("response.data: %ld\n", *(int*)response.response.data);
//    response.response.length = sizeof(ULONG);
//    response.response.chunk_id++;
//    PutMsg(port, &response);
//    WaitPort(response.msg.mn_ReplyPort);
//    GetMsg(response.msg.mn_ReplyPort);
//    FreeVec(response.response.data);
//  }
//
//  response.response.length = 0;
//  PutMsg(port, &response);
//  WaitPort(response.msg.mn_ReplyPort);
//
//  DeleteMsgPort(response.msg.mn_ReplyPort);
//}