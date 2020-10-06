file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${WhispCLI_SOURCE_DIR}/include/whisp-cli/*.h")
file(GLOB SRC_LIST "*.cc")

add_library(whisp-cli-lib ${SRC_LIST} ${HEADER_LIST})
target_include_directories(whisp-cli-lib PUBLIC ../include)

add_executable(whisp-cli main.cc)
target_compile_features(whisp-cli PRIVATE cxx_std_17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(whisp-cli PRIVATE whisp-cli-lib Threads::Threads)