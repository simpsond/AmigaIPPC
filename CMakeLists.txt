cmake_minimum_required(VERSION 3.27)
project(AmigaIPPC C)

set(CMAKE_C_STANDARD 99)

add_executable(ipc_ctrl
    examples/ipc_ctrl.c
    src/ippc.c
)

add_executable(ipc_srvc
    examples/ipc_srvc.c
    src/ippc.c
)
target_include_directories(ipc_ctrl PUBLIC src)
target_include_directories(ipc_srvc PUBLIC src)
target_link_options(ipc_ctrl PUBLIC)
#target_link_options(ipc_ctrl PUBLIC -nostdlib -nostartfiles)
#target_link_options(ipc_srvc PUBLIC -nostdlib -nostartfiles)
target_link_options(ipc_srvc PUBLIC -nostartfiles)
target_link_libraries(ipc_srvc PUBLIC 'debug')

install(TARGETS ipc_srvc ipc_ctrl DESTINATION ./)
install(DIRECTORY src DESTINATION ./)


