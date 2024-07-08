//
// Created by Dustin Simpson on 7/3/24.
//
#include <stdio.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <string.h>

#include "ipc.h"
#include "ippc.h"

#define COMMAND_COUNT 4

struct ExecBase* SysBase;
struct DosLibrary* DOSBase;
BPTR seglist;
struct Process *proc;
struct RDArgs *rd;
LONG params[4];

const unsigned char* task_name = "ipc_srvc";

struct Cmd {
  STRPTR name;
  void(*cmd)();
};

void StartSrvc();
void StopSrvc();
void SrvcStatus();
void SrvcGetRandom();

int IsSrvcRunning(struct Task** task);


int main() {
//  struct Cmd commands[COMMAND_COUNT];

  Printf("=== IPC Controller ===\n");

  SysBase = *(struct ExecBase**)4;
  DOSBase = (struct DosLibrary*)OpenLibrary("dos.library", 0);

  char template[] = "CMD/A";

  struct Cmd commands[] = {
      {.name = (STRPTR)"start", .cmd = StartSrvc},
      {.name = (STRPTR)"stop", .cmd = StopSrvc},
      {.name = (STRPTR)"status", .cmd = SrvcStatus},
      {.name = (STRPTR)"get_random", .cmd = SrvcGetRandom},
  };

  for (int i=0; i < 4; i++) {
    params[i] = 0;
  }


  rd = ReadArgs(template, params, NULL);

  if(!rd) {
    Printf("Format: ipc_ctrl CMD\n");
    return;
  }

  for(int i = 0; i < COMMAND_COUNT; i++) {
    if(strcmp((const char*)params[0], (char*)commands[i].name) == 0) {
      commands[i].cmd();
    }
  }

  FreeArgs(rd);
  return 0;
}

void StartSrvc() {
  struct Task* task;
  if(IsSrvcRunning(&task)) {
    Printf("service already running\n");
    return;
  }
  seglist = LoadSeg("ipc_srvc");
  if(!seglist) {
    Printf("unable to load ipc service\n");
    return 0;
  }

  proc = CreateNewProcTags(NP_Seglist, seglist, NP_Name, (BPTR)task_name, TAG_END);
}

void StopSrvc() {
  struct Task* task;
  if(IsSrvcRunning(&task)) {
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
  *task = FindTask(task_name);
  return *task != 0;
}

void GetRandomCB(struct CommandResponse* response) {
  Printf("%ld\n", *(int*)response->response->data);
}

void SrvcGetRandom() {
  struct IPPCRequestMsg cmd;
  struct Task* task;

  USHORT count_random_numbers = 2;

  if(IsSrvcRunning(&task)) {
    CreateCommandMessage(&cmd, (STRPTR) "get_random", (void*)&count_random_numbers, sizeof(USHORT));
//    cmd.command = (STRPTR) "get_random";
    CallTaskRPC(task, &cmd, GetRandomCB);
  }
}


