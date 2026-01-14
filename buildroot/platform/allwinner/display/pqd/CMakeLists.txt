cmake_minimum_required(VERSION 2.8)
project(pqd)

set(CMAKE_SYSTEM_NAME  Linux)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CROSS_COMPILE      $ENV{CROSS_COMPILE})
set(CMAKE_C_COMPILER   ${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)

set(CMAKE_C_FLAGS "-mfloat-abi=softfp -mfpu=vfpv3")
set(CMAKE_C_FLAGS "-std=gnu99")

list(APPEND EXTRA_LIBS pthread m)

include_directories(utils ./)

add_executable(pqd
    pqdaemon.c
    pq_core.c

    utils/utils.c
    utils/debug.c
    utils/cmdqueue.c
    utils/helper.c

    pqdata/pqdata.c

    sockets/socket.c
    sockets/socket_client.c

    hardwares/hardwares.c
    hardwares/de20x/de20x.c
    hardwares/de20x/fcm/fcm.c
    hardwares/de20x/fcm/constants.c
    )

target_link_libraries(pqd PUBLIC ${EXTRA_LIBS})

add_executable(client
    test/client.c

    utils/utils.c
    utils/debug.c
    sockets/socket.c
    )

target_link_libraries(client PUBLIC ${EXTRA_LIBS})

add_executable(fcm-test
    test/fcm-test.c
    utils/utils.c
    utils/debug.c
    sockets/socket.c
    sockets/socket_client.c
    hardwares/de20x/fcm/fcm.c
    hardwares/de20x/fcm/constants.c
    )

target_link_libraries(fcm-test PUBLIC ${EXTRA_LIBS})

