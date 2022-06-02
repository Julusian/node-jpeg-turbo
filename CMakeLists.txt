project (jpeg-turbo)

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=7)

cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0042 NEW)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  # Ensure jpeg-turbo is built without needing vc runtimes
  set(WITH_CRT_DLL FALSE)
  set(CMAKE_CXX_FLAGS_RELEASE "/MT")
  set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

include_directories(${CMAKE_JS_INC})

set(HEADER_FILES
  "src/buffersize.h"
  "src/compress.h"
  "src/consts.h"
  "src/decompress.h"
  "src/enums.h"
  "src/util.h"
)
set(SOURCE_FILES
  "src/buffersize.cc"
  "src/compress.cc"
  "src/decompress.cc"
  "src/enums.cc"
  "src/util.cc"
  "src/exports.cc"
)

option(WITH_JPEG8 "" 1)
option(ENABLE_SHARED "" 0)
add_subdirectory("deps/libjpeg-turbo")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(${PROJECT_NAME} PRIVATE "deps/libjpeg-turbo")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} turbojpeg-static)

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})
