# AmigaIPPC
Just because someone posts code to GitHub doesn't mean it's good, and that applies here. The work in this repository is me getting back into hobby Amiga development. If you see something and wonder if I did it wrong, the answer is probably, yes. Please feel free to point out problems or submit changes.

AmigaIPPC (Amiga Inter-Process Proceedure Call) is a framework for abstracting away Amiga's executive (Exec) message 
passing system. This is particularly useful in situations where a single request can yield multiple chunked responses.

## Compilers tested
* [m68k-amigaos-gcc](https://github.com/bebbo/amiga-gcc/) (GCC) 6.5.0b 221128234224
* SAS/C Amiga Compiler 6.58

### m68k-amigaos-gcc
To build, you can use CMake and the supplied toolchain config. From this project's root directory just
```
mkdir cmake-build
cmake -B cmake-build -DCMAKE_TOOLCHAIN_FILE=m68k-amigaos.cmake
cd cmake-build
make
```
example binaries will be in the cmake-build directory

### SAS/C Amiga Compiler 6.58
Assuming SAS/C is in your Amiga path then, from this project's root directory
```
makedir build
smake
```
example binaries will be in the build directory.

## Usage
Just include `src/ippc.h` and compile in `src/ippc.c`
### Client/Calling side
1. Use `CreateIPPCRequest` to create a request
2. Make the request with `CallTaskRPC`
3. Free resources created with `FreeIPPCRequest`

**Example**
```
struct IPPCRequest* request;

request = CreateIPPCRequest(command_name, (void*)&count_random_numbers, sizeof(USHORT));
CallTaskRPC((struct Process*)task, request, GetRandomCB);
FreeIPPCRequest(request);
```