file(GLOB HEADER_LIST CONFIGURE_DEPENDS
  "${WhispCLI_SOURCE_DIR}/include/whisp-cli/*.h"
  "${WhispCLI_SOURCE_DIR}/include/whisp-protobuf/cpp/*.h")
file(GLOB SRC_LIST
  "*.cc"
  "${WhispCLI_SOURCE_DIR}/include/whisp-protobuf/cpp/*.cc")

add_library(whisp-cli-lib ${SRC_LIST} ${HEADER_LIST})
target_include_directories(whisp-cli-lib PUBLIC ../include
  ${GTK3_INCLUDE_DIRS} ${GTKMM_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS} ${GTKMM_LIBRARY_DIRS})

add_executable(whisp-cli main.cc)
target_compile_features(whisp-cli PRIVATE cxx_std_17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 gtk+-3.0 REQUIRED)
pkg_check_modules(GTKMM gtkmm-3.0 REQUIRED)
if(WIN32)
  target_link_libraries(whisp-cli
    PRIVATE Threads::Threads whisp-cli-lib ws2_32 ${PROTOBUF_LIBRARY})
else()
  target_link_libraries(whisp-cli
    PRIVATE Threads::Threads whisp-cli-lib ${PROTOBUF_LIBRARY}
    ${GTK3_LIBRARIES} ${GTKMM_LIBRARIES})
endif()
