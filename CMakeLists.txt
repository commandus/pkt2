cmake_minimum_required(VERSION 2.8)
OPTION(WITH_DEBUG_MODE "Build with debug mode" ON)

project(pkt2)
set(pkt2_VERSION_MAJOR 0)
set(pkt2_VERSION_MINOR 1)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Argtable2 REQUIRED)
find_package(Glog REQUIRED)
find_package(Nanomsg REQUIRED)

include_directories(. ${ARGTABLE2_INCLUDE_DIRS} ${GLOG_INCLUDE_DIRS} ${NANOMSG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ARGTABLE2_LIBRARIES} ${GLOG_LIBRARIES} ${NANOMSG_LIBRARIES})

set(cpp_tcpreceiver tcpreceiver.cpp  tcpreceiver-config.cpp 
    daemonize.cpp  ieee754.cpp  utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp 
)

set(cpp_pkt2receiver pkt2receiver.cpp  pkt2receiver-config.cpp 
    daemonize.cpp  ieee754.cpp  utilpriority.cpp  utilstring.cpp utilinet.cpp NanoMessage.cpp 
)

add_executable(tcpreceiver ${cpp_tcpreceiver})
add_executable(pkt2receiver ${cpp_pkt2receiver})
