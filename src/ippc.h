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

#ifndef AMIGAIPCDEMO_SRC_IPPC_H_
#define AMIGAIPCDEMO_SRC_IPPC_H_

#include <exec/ports.h>
#include <exec/types.h>
#include <dos/dosextens.h>

/**
 * @brief structure used to make a request to another process
 *
 */
struct IPPCRequest {
  struct MsgPort* response_port; /**< The port that will listen for the response(s) to the request */
  STRPTR command_name; /**< String that can be used to identify the payload type or command to invoke */
  ULONG payload_sz; /**< Size of the memory allocated for the void* payload */
  void* payload; /**< Payload data */
};

/**
 * @brief structure used for a response to a request.  Since it is assumed that a single request can have the response
 * broken into chunks, a chunk_id field is provided.
 */
struct IPPCResponse {
  USHORT chunk_id; /**< conventionally, chunk_id = 0 means not chunked. Likewise, terminating response will have this 0 */
  ULONG length; /**< length of the void* data response. Terminating response will have this as 0 */
  void* data; /**< response data, terminating response will have this as NULL */
};

/**
 * @brief extension of exec message with IPPCRequest
 */
struct RequestMessage {
  struct Message msg; /**< exec message structure */
  struct IPPCRequest* request; /**< request body */
};

/**
 * @brief extension of exec message with IPCResponse
 */
struct CommandResponse {
  struct Message msg; /**< exec message structure */
  struct IPPCResponse* response; /**< response body */
};

/**
 * @brief Creates an inter-process procedure call request. Expectation is that the process that
 * creates the request will free allocated memory by calling FreeIPPCRequest
 *
 * @param[in] command name of the command that this request will invoke
 * @param[in] data whatever payload data that needs to be sent to the remote process
 * @param[in] sz size of the payload data
 *
 * @return Pointer to the request, must be deallocated with FreeIPPCRequest
 */
struct IPPCRequest* CreateIPPCRequest( STRPTR command, void* data, ULONG sz);

/**
 * @brief Frees memory previously allocated by CreateIPPCRequest
 * @param[in] request structure previously allocated with CreateIPPCRequest to be freed
 */
void FreeIPPCRequest(struct IPPCRequest* request);

/**
 * @brief Used to call the remote procedure through a process's default message port
 *
 * This is just a wrapper around CallPortRPC that allows you to pass in a process instead of going through all the
 * trouble of typing, `&proc->pr_MsgPort`
 * @param[in] proc - that has it's default message port listening for a command to be called
 * @param[in] cmd - constructed with CreateIPPCRequest, the assumption is that this request will be valid until
 * the remote side has fully replied.
 * @param[in] cb - for the response from the remote side.
 */
void CallTaskRPC(struct Process* proc, struct IPPCRequest* cmd, void(*cb)(struct CommandResponse* data));

/**
* @brief Used to call the remote procedure through the specified message port
*
* The specified message port should be constructed specifically for #RequestMessage
*
* @param[in] port - message port listening for a command to be called
* @param[in] cmd - constructed with CreateIPPCRequest, the assumption is that this request will be valid until
* the remote side has fully replied.
* @param[in] cb - for the response from the remote side.
*/
void CallPortRPC(struct MsgPort* port, struct IPPCRequest* cmd, void(*cb)(struct CommandResponse* data));

/**
 * @brief This function will check the specified port for a message/command and attempt to service the command
 *
 * @warning If a message is waiting on the port it will be blindly treated as a #RequestMessage
 *
 * @param[in] port - message port to check for a message
 * @param[in] OnCommand - callback to process the incoming command message
 */
void RPCGetCommand(struct MsgPort* port, void(*OnCommand)(struct IPPCRequest*, void(*CB)(struct IPPCRequest* request, struct IPPCResponse* response)));


#endif //AMIGAIPCDEMO_SRC_IPPC_H_
