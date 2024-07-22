//
// Created by Dustin Simpson on 7/4/24.
//
#include <proto/dos.h>
#include <proto/exec.h>
//#include <clib/debug_protos.h>
#include <string.h>
#include <exec/memory.h>

#include "ippc.h"

#ifdef __GNUC__
struct ExecBase* SysBase;
struct DosLibrary* DOSBase;
#endif

#ifdef __GNUC__
#define __saveds
asm("bra _entry");
#endif

//void CmdGetRandom(struct MsgPort* port, int count);
void __saveds entry(void);
void OnReceivedCommand(struct IPPCRequest* request, void(*cb)(struct IPPCRequest* request, struct IPPCResponse* response));

void __saveds entry() {
  struct Process* self_process;


#ifdef __GNUC__
  SysBase = *(struct ExecBase**)4;
#endif
  DOSBase = (struct DosLibrary*)OpenLibrary("dos.library", 0);
  self_process = (struct Process*)FindTask(NULL);

  for(;;) {
    #ifdef ENABLE_KPRINT
    KPrintF("ipc_srvc ping\n");
    #endif
    RPCGetCommand(&self_process->pr_MsgPort, OnReceivedCommand);
    Delay(10);
  }
}


void OnReceivedCommand(struct IPPCRequest* request, void(*cb)(struct IPPCRequest* request, struct IPPCResponse* response)) {
  if(strcmp("get_random", (const char*)request->command_name) == 0) {
    USHORT how_many;
    int i;
    struct IPPCResponse response;
    response.length = sizeof(ULONG);
    response.data = AllocMem(response.length, MEMF_ANY | MEMF_CLEAR);
    how_many = *(USHORT *)request->payload;
    for(i = 0; i < how_many; i++) {
      *(ULONG*)response.data = 42;
      response.chunk_id = i+1;
      cb(request, &response);
    }
    FreeMem(response.data, response.length);
  }
}