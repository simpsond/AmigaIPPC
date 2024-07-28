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

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>

#include "ippc.h"

static struct Process* pong_process;
static struct Process* main_process;

static BYTE quit_sig;
static BYTE main_sig;

static void OnPongCmd(struct IPPCRequest* request, void(*cb)(struct IPPCRequest* request, struct IPPCResponse* response)) {
  struct IPPCResponse response;
  const unsigned char* pong_str = "pong";
  response.data = pong_str;
  response.length = 5;
  response.chunk_id = 0;
  cb(request, &response);
}

__saveds static void PongProcess() {
  int exit = 0;
#ifdef ENABLE_KPRINT
  KPrintF("Entered PongProcess\n");
#endif
  quit_sig = AllocSignal(-1);
  while(!exit) {
    RPCGetCommand(&pong_process->pr_MsgPort, OnPongCmd);
    if(CheckSignal(1 << quit_sig)) {
      exit = 1;
    }
#ifdef ENABLE_KPRINT
    KPrintF("PongProcess\n");
#endif
    Delay(1);
  }
  Signal(main_process, 1<<main_sig);
}

static void PingCB(struct CommandResponse* response) {
  if(response->response->length) {
    Printf("** %s **\n", response->response->data);
  }
}

int main(void) {
  STRPTR task_name = "Pong Process";
  STRPTR ping_str = "ping";

  struct IPPCRequest* request;
  Printf("Ping Pong basic ippc demo....\n");

  Printf("Starting Pong Process\n");
  main_sig = AllocSignal(-1);
  main_process = (struct Process*)FindTask(NULL);
  pong_process = CreateNewProcTags(NP_Entry, PongProcess, NP_Name, task_name, TAG_END);
  if(pong_process) {
    Printf("Pong process started\n");
  } else {
    Printf("Couldn't start Pong process\n");
    return -1;
  }

  request = CreateIPPCRequest(ping_str, NULL, 0);

  Printf("Going to CallTaskRPC\n");
  CallTaskRPC(pong_process, request, PingCB);
  FreeIPPCRequest(request);

  Signal((struct Task*)pong_process, 1 << quit_sig);
  Wait(1 << main_sig);
  return 0;
}