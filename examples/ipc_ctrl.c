//
// Created by Dustin Simpson on 7/3/24.
//

#include <proto/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>

#include <string.h>

#include "ippc.h"


#ifdef __GNUC__
struct ExecBase* SysBase;
struct DosLibrary* DOSBase;
#endif


const unsigned char* task_name = "ipc_srvc";

struct Cmd {
  STRPTR name;
  void(*cmd)(void);
};

void StartSrvc(void);
void StopSrvc(void);
void SrvcStatus(void);
void SrvcGetRandom(void);
int IsSrvcRunning(struct Task** task);



int main() {
  int i;
  struct RDArgs *rd;
  LONG params[] = {0, 0, 0, 0};
  char template[] = "CMD/A";

  struct Cmd commands[] = {
      {(STRPTR)"start", StartSrvc},
      {(STRPTR)"stop", StopSrvc},
      {(STRPTR)"status", SrvcStatus},
      {(STRPTR)"get_random", SrvcGetRandom},
  };
  UBYTE command_count = sizeof(commands)/sizeof(commands[0]);

#ifdef __GNUC__
  SysBase = *(struct ExecBase**)4;
  DOSBase = (struct DosLibrary*)OpenLibrary("dos.library", 0);
#endif

  Printf("=== IPC Controller ===\n");

  rd = ReadArgs(template, params, NULL);

  if(!rd) {
    Printf("Format: ipc_ctrl CMD\n");
    return -1;
  }

  for(i = 0; i < command_count; i++) {
    if(strcmp((const char*)params[0], (char*)commands[i].name) == 0) {
      commands[i].cmd();
    }
  }

  FreeArgs(rd);
  return 0;
}

void StartSrvc() {
  struct Task* task;
  struct Process *proc;
  BPTR seglist;

  if(IsSrvcRunning(&task)) {
    Printf("service already running\n");
    return;
  }
  seglist = LoadSeg("ipc_srvc");
  if(!seglist) {
    Printf("unable to load ipc service\n");
    return;
  }

  proc = CreateNewProcTags(NP_Seglist, seglist, NP_Name, (BPTR)task_name, TAG_END);
}

void StopSrvc() {
  struct Task* task;
  Printf("In StopSrvc\n");
  if(IsSrvcRunning(&task)) {
    Printf("Task is running\n");
    RemTask(task);
    Printf("called RemTask on ipc_srvc\n");
  } else {
    Printf("couldn't find ipc_srvc to remove it\n");
  }
}

void SrvcStatus() {
  struct Task* task;
  if(IsSrvcRunning(&task)) {
    Printf("ipc_srvc is running\n");
  } else {
    Printf("ipc_srvc is not running\n");
  }
}

int IsSrvcRunning(struct Task** task) {
  *task = FindTask((STRPTR)task_name);
  return *task != 0;
}

void GetRandomCB(struct CommandResponse* response) {
  if(response->response->length) {
    Printf("%ld\n", *(int*)response->response->data);
  } else {
    Printf("all done!\n");
  }
}

void SrvcGetRandom() {
  struct IPPCRequest* request;
  struct Task* task;

  USHORT count_random_numbers = 2;

  unsigned char command_name[] = "get_random";

  if(IsSrvcRunning(&task)) {
    request = CreateIPPCRequest(command_name, (void*)&count_random_numbers, sizeof(USHORT));
    Printf("going to call: CallTaskRPC\n");
    CallTaskRPC((struct Process*)task, request, GetRandomCB);
    FreeIPPCRequest(request);
  }
}


