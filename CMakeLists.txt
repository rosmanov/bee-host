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

# cJSON and cJSONUtils
ExternalProject_Add(cjson
  GIT_REPOSITORY "https://github.com/DaveGamble/cJSON"
  GIT_TAG "v1.7.10"
  PREFIX "${CMAKE_SOURCE_DIR}/external/cJSON"
  BUILD_IN_SOURCE 1
  TEST_COMMAND ""
  INSTALL_COMMAND ""
  CMAKE_ARGS "-DBUILD_SHARED_AND_STATIC_LIBS=On -DBUILD_SHARED_LIBS=Off -DENABLE_CJSON_TEST=Off -DENABLE_CJSON_UTILS=On"
)
ExternalProject_Get_Property(cjson source_dir)
ExternalProject_Get_Property(cjson binary_dir)
ExternalProject_Get_Property(cjson download_dir)
set(CJSON_SOURCE_DIR "${source_dir}")
set(CJSON_BINARY_DIR "${binary_dir}")
set(CJSON_DOWNLOAD_DIR "${download_dir}")

# beectl
set(BEECTL_SRCS
  src/beectl.c
  src/shell.c
  src/str.c
  src/io.c
  "${CJSON_SOURCE_DIR}/cJSON.c")
add_executable(beectl ${BEECTL_SRCS})
target_include_directories(beectl PRIVATE "${CJSON_DOWNLOAD_DIR}")
add_dependencies(beectl cjson)

# json-patch
set(BEECTL_JSON_REPLACE_SRCS
  src/json-patch.c
  src/io.c
  "${CJSON_SOURCE_DIR}/cJSON.c"
  "${CJSON_SOURCE_DIR}/cJSON_Utils.c")
add_executable(json-patch ${BEECTL_JSON_REPLACE_SRCS})
target_include_directories(json-patch PRIVATE "${CJSON_DOWNLOAD_DIR}")
add_dependencies(json-patch cjson)

# CPack common properties
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
if("${CMAKE_TOOLCHAIN_FILE}" MATCHES "Linux")
    set(CPACK_GENERATOR RPM DEB)
elseif("${CMAKE_TOOLCHAIN_FILE}" MATCHES "Windows")
  set(CPACK_GENERATOR "NSIS")
endif()
set(CPACK_PACKAGE_NAME "beectl")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Ruslan Osmanov <rrosmanov@gmail.com>")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${PROJECT_NAME}")
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_FILE_NAME
  "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.${CMAKE_BUILD_TYPE}")

# RPM package properties
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/beectl.spec.in"
  "${CMAKE_CURRENT_BINARY_DIR}/beectl.spec"
  @ONLY IMMEDIATE)
set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/beectl.spec")

# NSIS package properties
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake/modules/nsis" ${CMAKE_MODULE_PATH})
set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_INSTALLED_ICON_NAME "beectl.ico")
set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_DESCRIPTION}")
set(CPACK_NSIS_HELP_LINK "https://github.com/rosmanov/chrome-bee")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/rosmanov/chrome-bee")
set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
configure_file(
  ${PROJECT_SOURCE_DIR}/CMake/modules/nsis/NSIS.definitions.nsh.in
  ${CMAKE_CURRENT_BINARY_DIR}/CMake/modules/nsis/NSIS.definitions.nsh
)
include(CPack)
