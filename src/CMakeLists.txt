file(GLOB HEADER_LIST CONFIGURE_DEPENDS
  "${WhispCLI_SOURCE_DIR}/include/whisp-cli/*.h"
  "${WhispCLI_SOURCE_DIR}/include/whisp-protobuf/cpp/*.h")
file(GLOB SRC_LIST
  "*.cc"
  "${WhispCLI_SOURCE_DIR}/include/whisp-protobuf/cpp/*.cc")

add_library(whisp-cli-lib ${SRC_LIST} ${HEADER_LIST})
target_include_directories(whisp-cli-lib PUBLIC ../include)

add_executable(whisp-cli main.cc)
target_compile_features(whisp-cli PRIVATE cxx_std_17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

if(WIN32)
  target_link_libraries(whisp-cli
    PRIVATE Threads::Threads whisp-cli-lib ws2_32 ${PROTOBUF_LIBRARY})
else()
  target_link_libraries(whisp-cli
    PRIVATE Threads::Threads whisp-cli-lib ${PROTOBUF_LIBRARY})
endif()
