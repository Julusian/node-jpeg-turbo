cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW) # DOWNLOAD_EXTRACT_TIMESTAMP
endif()
include(ExternalProject)

project (jpeg-turbo)

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=10)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_JS_INC})

set(SOURCE_FILES
  "src/buffersize.cc"
  "src/buffersize.h"
  "src/compress.cc"
  "src/compress.h"
  "src/decompress.cc"
  "src/decompress.h"
  "src/enums.cc"
  "src/enums.h"
  "src/util.cc"
  "src/util.h"
  "src/exports.cc"
  "src/consts.h"
)

set(libjpeg-turbo_version "3.1.3")
set(libjpeg-turbo_url "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${libjpeg-turbo_version}/libjpeg-turbo-${libjpeg-turbo_version}.tar.gz")
set(libjpeg-turbo_sha256 "075920b826834ac4ddf97661cc73491047855859affd671d52079c6867c1c6c0")

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
set(LIBJPEG_TURBO_OPTIONS)
if(CMAKE_GENERATOR)
  list(APPEND LIBJPEG_TURBO_OPTIONS CMAKE_GENERATOR "${CMAKE_GENERATOR}")
endif()
if(CMAKE_GENERATOR_PLATFORM)
  list(APPEND LIBJPEG_TURBO_OPTIONS CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}")
endif()
if(CMAKE_GENERATOR_TOOLSET)
  list(APPEND LIBJPEG_TURBO_OPTIONS CMAKE_GENERATOR_TOOLSET "${CMAKE_GENERATOR_TOOLSET}")
endif()

ExternalProject_Add(libjpeg-turbo
  URL ${libjpeg-turbo_url}
  URL_HASH SHA256=${libjpeg-turbo_sha256}
  CMAKE_ARGS -DENABLE_SHARED=${ENABLE_SHARED} -DENABLE_STATIC=${ENABLE_STATIC} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} -DREQUIRE_SIMD=ON ${LIBJPEG_TURBO_OPTIONS}
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(libjpeg-turbo SOURCE_DIR)
set(JPEG_INCLUDE_DIR ${SOURCE_DIR}/src)
ExternalProject_Get_Property(libjpeg-turbo BINARY_DIR)
set(JPEG_LIB_DIR ${BINARY_DIR})
set(JPEG_LIB turbojpeg)

if (MSVC)
  # Windows uses a different path for some reason
  set(JPEG_LIB_DIR ${JPEG_LIB_DIR}/Release)
  set(JPEG_LIB turbojpeg-static)
endif()


set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(${PROJECT_NAME} PRIVATE ${JPEG_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${JPEG_LIB_DIR})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${JPEG_LIB})
add_dependencies(${PROJECT_NAME} libjpeg-turbo)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

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
