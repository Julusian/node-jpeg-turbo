cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)
# cmake_policy(SET CMP0135 NEW)
include(ExternalProject)

project (jpeg-turbo)

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=7)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

set(libjpeg-turbo_version "3.0.1")
set(libjpeg-turbo_url "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/${libjpeg-turbo_version}.tar.gz")
set(libjpeg-turbo_md5 "88a0b4f9ad57924ce064135a1717c703")

if(UNIX AND NOT APPLE)
  # build shared on linux, to avoid conflicts with libjpeg built into electron
  set(ENABLE_SHARED 1)
  set(ENABLE_STATIC 0)

else()
  # build static elsewhere
  set(ENABLE_SHARED 0)
  set(ENABLE_STATIC 1)
endif()

# Download and build libjpeg-turbo
ExternalProject_Add(libjpeg-turbo
  URL ${libjpeg-turbo_url}
  URL_MD5 ${libjpeg-turbo_md5}
  CMAKE_ARGS -DENABLE_SHARED=${ENABLE_SHARED} -DENABLE_STATIC=${ENABLE_STATIC} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(libjpeg-turbo SOURCE_DIR)
set(JPEG_INCLUDE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(libjpeg-turbo BINARY_DIR)
set(JPEG_LIB_DIR ${BINARY_DIR})
set(JPEG_LIB turbojpeg)

if (MSVC)
  # Windows uses a different path for some reason
  set(JPEG_LIB_DIR ${JPEG_LIB_DIR}/Release)
  set(JPEG_LIB turbojpeg-static)
endif()

message("JP ${JPEG_LIB_DIR}")

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(${PROJECT_NAME} PRIVATE ${JPEG_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${JPEG_LIB_DIR})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${JPEG_LIB})
add_dependencies(${PROJECT_NAME} libjpeg-turbo)

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

if(UNIX AND NOT APPLE)
  # copy built lib into build dir
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${JPEG_LIB_DIR}/libturbojpeg.so.0 ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
  )
endif()
