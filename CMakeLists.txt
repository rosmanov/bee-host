cmake_minimum_required(VERSION 3.9)
# Policies are written for CMake version 3.9.6
cmake_policy(VERSION 3.9.6)
# CMP0048 set to NEW allows setting project VERSION via project() command
cmake_policy(SET CMP0048 NEW)

project(BeeCtl
  VERSION 1.0.0
  DESCRIPTION "Native Messaging Host for Bee Extension"
  LANGUAGES C)

# C11
set(CMAKE_CXX_STANDARD 11)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(ExternalProject)

check_c_source_compiles("
int main(void) { if (__builtin_expect(0, 0)) return 1; return 0; }
" HAVE___BUILTIN_EXPECT)

check_function_exists(strndup HAVE_STRNDUP)
if(HAVE_STRNDUP)
  add_definitions(-DHAVE_STRNDUP)
endif()

set(BEECTL_SRCS beectl.c shell.c str.c io.c)
set(LIBS "")

# Directory for external projects
set (EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

# cJSON
set(CJSON_DIR "${EXTERNAL_DIR}/cJSON")
ExternalProject_Add(cjson
  GIT_REPOSITORY "https://github.com/DaveGamble/cJSON"
  GIT_TAG "v1.7.10"
  PREFIX "${CJSON_DIR}"
  BUILD_IN_SOURCE 1
  TEST_COMMAND ""
  INSTALL_COMMAND ""
  CMAKE_ARGS "-DBUILD_SHARED_AND_STATIC_LIBS=On -DBUILD_SHARED_LIBS=Off -DENABLE_CJSON_TEST=Off"
)
ExternalProject_Get_Property(cjson source_dir)
ExternalProject_Get_Property(cjson binary_dir)
set(CJSON_SOURCE_DIR "${source_dir}")
set(CJSON_BINARY_DIR "${binary_dir}")
set(BEECTL_SRCS ${BEECTL_SRCS} "${CJSON_SOURCE_DIR}/cJSON.c")
set(LIBS ${LIBS} "${CJSON_BINARY_DIR}/libcjson${CMAKE_STATIC_LIBRARY_SUFFIX}")
include_directories("${CJSON_SOURCE_DIR}")

# JSON parser
#set(JSON_PARSER_DIR "${EXTERNAL_DIR}/json-parser")
#ExternalProject_Add(libjsonparser
  #GIT_REPOSITORY "https://github.com/udp/json-parser.git"
  #GIT_TAG "b42439a2927a879f40698e4861e727c4265c13e6"
  #PREFIX "${JSON_PARSER_DIR}"
  #BUILD_IN_SOURCE 1
  #CONFIGURE_COMMAND
  #"${JSON_PARSER_DIR}/src/libjsonparser/configure"
  #--prefix=<INSTALL_DIR>
  #"CC=${CMAKE_C_COMPILER}"
  #"CFLAGS=${CMAKE_C_FLAGS}"
  ##--build=x86_64-pc-linux-gnu
  #--host=${BEECTL_HOST}
  #TEST_COMMAND ""
  #INSTALL_COMMAND ""
#)
#ExternalProject_Get_Property(libjsonparser source_dir)
#ExternalProject_Get_Property(libjsonparser binary_dir)
#set(JSON_PARSER_SOURCE_DIR "${source_dir}")
#set(JSON_PARSER_BINARY_DIR "${binary_dir}")
#set(BEECTL_SRCS ${BEECTL_SRCS} "${JSON_PARSER_SOURCE_DIR}/json.c")
#set(LIBS ${LIBS} "${JSON_PARSER_BINARY_DIR}/libjsonparser${CMAKE_STATIC_LIBRARY_SUFFIX}")
#include_directories("${JSON_PARSER_SOURCE_DIR}")

# JSON builder
#set(JSON_BUILDER_DIR "${EXTERNAL_DIR}/json-builder")
#ExternalProject_Add(libjsonbuilder
  #GIT_REPOSITORY "https://github.com/udp/json-builder.git"
  #GIT_TAG "e159ecd340983c2ff9165a465c9477b08ba8a465"
  #PREFIX "${JSON_BUILDER_DIR}"
  #BUILD_IN_SOURCE 1
  #CMAKE_ARGS "-DCMAKE_C_FLAGS=-I'${JSON_PARSER_SOURCE_DIR}'"
  #TEST_COMMAND ""
  #INSTALL_COMMAND ""
#)
#ExternalProject_Get_Property(libjsonbuilder source_dir)
#ExternalProject_Get_Property(libjsonbuilder binary_dir)
#set(JSON_BUILDER_SOURCE_DIR "${source_dir}")
#set(JSON_BUILDER_BINARY_DIR "${binary_dir}")
#set(BEECTL_SRCS ${BEECTL_SRCS} "${JSON_BUILDER_SOURCE_DIR}/json-builder.c")
#set(LIBS ${LIBS} "${JSON_BUILDER_BINARY_DIR}/libjsonbuilder${CMAKE_STATIC_LIBRARY_SUFFIX}")
#include_directories("${JSON_BUILDER_SOURCE_DIR}")

add_executable(beectl ${BEECTL_SRCS})
target_include_directories(beectl PRIVATE ${EXTERNAL_DIR})
#add_dependencies(libjsonbuilder libjsonparser)
add_dependencies(
  beectl
  cjson
)
target_link_libraries(beectl m)

#set(BEECTL_JSON_REPLACE_SRCS json-replace.c io.c
  #"${JSON_PARSER_SOURCE_DIR}/json.c"
  #"${JSON_BUILDER_SOURCE_DIR}/json-builder.c")
#add_executable(json-replace ${BEECTL_JSON_REPLACE_SRCS})
#target_include_directories(beectl PRIVATE ${EXTERNAL_DIR})
#add_dependencies(
  #json-replace
  #libjsonparser
  #libjsonbuilder
#)
#target_link_libraries(json-replace m)

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
if("${CMAKE_TOOLCHAIN_FILE}" MATCHES "Linux")
  set(CPACK_GENERATOR "RPM")
elseif("${CMAKE_TOOLCHAIN_FILE}" MATCHES "Windows")
  set(CPACK_GENERATOR "NSIS")
endif()
set(CPACK_PACKAGE_NAME "beectl")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Ruslan Osmanov")

set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${PROJECT_NAME}")

set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

# RPM package
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/beectl.spec.in"
  "${CMAKE_CURRENT_BINARY_DIR}/beectl.spec"
  @ONLY IMMEDIATE)
set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/beectl.spec")

# NSIS package
set(CPACK_NSIS_INSTALLED_ICON_NAME "beectl.ico")
set(CPACK_NSIS_HELP_LINK "https://github.com/rosmanov/chrome-bee")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/rosmanov/chrome-bee")
set(CPACK_NSIS_CONTACT "rrosmanov@gmail.com")
configure_file(
  ${PROJECT_SOURCE_DIR}/NSIS.definitions.nsh.in
  ${CMAKE_CURRENT_BINARY_DIR}/NSIS.definitions.nsh
)

include(CPack)
