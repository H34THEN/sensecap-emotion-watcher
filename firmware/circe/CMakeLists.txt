cmake_minimum_required(VERSION 3.16)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(NOT DEFINED ENV{CIRCE_WATCHER_SDK})
    set(CIRCE_WATCHER_SDK "/tmp/sensecap-watcher-firmware")
else()
    set(CIRCE_WATCHER_SDK $ENV{CIRCE_WATCHER_SDK})
endif()

set(EXTRA_COMPONENT_DIRS
    "${CIRCE_WATCHER_SDK}/components"
    "${CMAKE_CURRENT_LIST_DIR}/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_options(-fdiagnostics-color=always -Wno-error=format= -Wno-format)

project(circe)
