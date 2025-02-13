cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(netpw)

option(NETPW_DEPLOYMENT "Compile with full optimizations." ON)
set(NETPW_PIPEWIRE_VERSION "0.3" CACHE STRING "Version of your PipeWire install.")
set(NETPW_SPA_VERSION "0.2" CACHE STRING "Version of your SPA install.")

add_definitions("-Wall -Wno-visibility")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

find_file(NETPW_PIPEWIRE_INCLUDE_PATH "pipewire-${NETPW_PIPEWIRE_VERSION}")
find_file(NETPW_SPA_INCLUDE_PATH "spa-${NETPW_SPA_VERSION}")

include_directories(${NETPW_PIPEWIRE_INCLUDE_PATH})
include_directories(${NETPW_SPA_INCLUDE_PATH})

if (NETPW_DEPLOYMENT)
    add_definitions("-O3")
else ()
    add_definitions("-g")
endif ()


file(GLOB NETPW_SOURCES "${PROJECT_SOURCE_DIR}/src/*.c" "${PROJECT_SOURCE_DIR}/src/*.cpp")
add_executable(netpw ${NETPW_SOURCES})
set_target_properties(netpw PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_custom_target(manual COMMAND mkdir -p "${PROJECT_SOURCE_DIR}/sys/share/man/man1" && gzip -c "${PROJECT_SOURCE_DIR}/sys/netpw.1" > "${PROJECT_SOURCE_DIR}/sys/share/man/man1/netpw.1.gz")
add_dependencies(netpw manual)

target_link_libraries(netpw ssl)
target_link_libraries(netpw crypto)
target_link_libraries(netpw pipewire-${NETPW_PIPEWIRE_VERSION})

install(TARGETS netpw DESTINATION bin)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/sys/share/" DESTINATION share)
