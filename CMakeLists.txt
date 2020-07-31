project (jpegturbo)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  # Ensure jpeg-turbo is built without needing vc runtimes
  set(WITH_CRT_DLL FALSE)
  set(CMAKE_CXX_FLAGS_RELEASE "/MT")
  set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

include_directories(${CMAKE_JS_INC})

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

if(napi_build_version)
  add_definitions(-DNAPI_VERSION=${napi_build_version})
endif()

set(HEADER_FILES
        "src/exports.h"
        "src/utils.h"
    )
set(SOURCE_FILES
        "src/buffersize.cc"
        "src/compress.cc"
        "src/decompress.cc"
        "src/exports.cc"
        "src/utils.cc"
    )

add_subdirectory("deps/libjpeg-turbo")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(${PROJECT_NAME}
  PRIVATE ${NODE_ADDON_API_DIR}
  PRIVATE "deps/libjpeg-turbo")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} turbojpeg-static)
