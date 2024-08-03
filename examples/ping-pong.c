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

/**
 * @file ping-pong.c
 * @author Dustin Simpson
 * @date 28 July 2024
 * @brief File contains a basic example of using IPPC
 */

#include <proto/dos.h>    //
#include <proto/exec.h>   // Amiga system includes needed for creating and
#include <dos/dostags.h>  // managing processes

#include "ippc.h"         // AmigaIPPC include file needed for IPPC functions

static struct Process* pong_process;  //< handle to the secondary "server" process that will respond to the call
static struct Task* main_task;  //< handle to main task needed to send a signal to us after pong process closes

static BYTE quit_sig; //< signal used to inform pong_process it should shut down
static BYTE main_sig; //< signal used by pong_process to inform the main program it is shutting down

/**
 * @brief Sends a "pong" IPPCResponse in response to an IPPCRequest
 * @param request an IPPCRequest*
 * @param cb Callback used to send back the response
 */
static void OnPongCmd(struct IPPCRequest* request, void(*cb)(struct IPPCRequest* request, struct IPPCResponse* response)) {
  struct IPPCResponse response;
  const unsigned char* pong_str = "pong";
  response.data = pong_str;
  response.length = 5;
  response.chunk_id = 0;
  cb(request, &response);
}

/**
 * @brief The main loop for a secondary "server" process that will loop waiting for a command
 * @note Since PongProcess is launched as a separate process, SAS/C needs __saveds to ensure
 * the address to the near data section in A4
 * @note From SAS/C Development System User's Guide Volume 1:
 * If you define a function with the __saveds keyword, the compiler generates extra code at the beginning of the function that loads the address of the near data section into register A4. Defining a function with the __saveds keyword is equivalent to compiling that function with the saveds compiler option.
 */
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
  Signal(main_task, 1<<main_sig);
}

/**
 * @brief Will be called from the main process with the response provided by the server, should print
 * ** Pong **
 */
static void PingCB(struct CommandResponse* response) {
  if(response->response->length) {
    Printf("** %s **\n", response->response->data);
  }
}

int main(void) {
  STRPTR task_name = "Pong Process";  // Set up a couple strings to be used later
  STRPTR ping_str = "ping";

  struct IPPCRequest* request;        //< request we will send to the server, this isn't usable until CreateIPPCRequest below
  Printf("Ping Pong basic ippc demo....\n");

  Printf("Starting Pong Process\n");
  main_sig = AllocSignal(-1); // http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01E9.html
  main_task = FindTask(NULL); // FindTask with NULL passed in will give us ourselves as a Task pointer

  // Start the server that will listen for IPPC requests, The CreateNewProc* functions are V36+
  // So this will not work on with Kickstart <= 1.3
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