cmake_minimum_required(VERSION 3.9)
# Policies are written for CMake version 3.9.6
cmake_policy(VERSION 3.9.6)
# CMP0048 set to NEW allows setting project VERSION via project() command
cmake_policy(SET CMP0048 NEW)
option(CPACK_GENERATOR "CPack generator" "RPM DEB TGZ ZIP productbuild")

project(BeeCtl
  VERSION 1.2.0
  DESCRIPTION "Native Messaging Host for Bee Extension"
  LANGUAGES C)

# C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(EXTERNAL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)

# cJSON and cJSONUtils
ExternalProject_Add(cjson
  GIT_REPOSITORY "https://github.com/DaveGamble/cJSON"
  GIT_TAG "v1.7.12"
  PREFIX "${EXTERNAL_DIR}/cJSON"
  BUILD_IN_SOURCE 1
  TEST_COMMAND ""
  INSTALL_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DBUILD_SHARED_AND_STATIC_LIBS=On
    -DBUILD_SHARED_LIBS=Off
    -DENABLE_CJSON_TEST=Off
    -DENABLE_CJSON_UTILS=On
    -DENABLE_TARGET_EXPORT=On
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
  # This is nasty, but I couldn't find a way to use CMAKE_TOOLCHAIN_FILE
  # for the external project (see comments below.)
  "${CJSON_SOURCE_DIR}/cJSON.c"
  )

add_executable(beectl ${BEECTL_SRCS})

# Workaround for CMake versions which require the cJSON.c file to exist before
# downloading the external project.
add_custom_command(TARGET beectl PRE_BUILD
    BYPRODUCTS "${CJSON_SOURCE_DIR}/cJSON.c"
    COMMAND touch "${CJSON_SOURCE_DIR}/cJSON.c")

target_compile_features(beectl PRIVATE c_variadic_macros)
target_include_directories(beectl PRIVATE "${CJSON_DOWNLOAD_DIR}")
set_property(TARGET beectl PROPERTY C_STANDARD 11) # C11
add_dependencies(beectl cjson)

# The following would be better approach than including cJSON files in our
# source list. But the build fails when cross-compiling, since
# ExternalProject_Add() does not regard the CMAKE_TOOLCHAIN_FILE. Passing
# CMAKE_TOOLCHAIN_FILE via CMAKE_ARGS didn't work either.
#target_link_libraries(beectl
    #"${CJSON_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}cjson${CMAKE_STATIC_LIBRARY_SUFFIX}")


# CPack common properties
set(CPACK_SET_DESTDIR "ON")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_NAME "beectl")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Ruslan Osmanov <rrosmanov@gmail.com>")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${PROJECT_NAME}")
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_FILE_NAME
  "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.${CMAKE_BUILD_TYPE}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(BEECTL_MANIFEST_TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin/beectl")
set(BEECTL_CHROME_MANIFEST_DIR "etc/opt/chrome/native-messaging-hosts")
set(BEECTL_FIREFOX_MANIFEST_DIR "usr/lib/mozilla/native-messaging-hosts")

# RPM package properties
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/beectl.spec.in"
  "${CMAKE_CURRENT_BINARY_DIR}/beectl.spec"
  @ONLY IMMEDIATE)
set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/beectl.spec")

# Install docs
if(NOT CMAKE_DOC_DIR)
    set(CMAKE_DOC_DIR share/doc/osmanov/beectl)
endif(NOT CMAKE_DOC_DIR)
if(NOT CMAKE_CONFIG_DIR)
    set(CMAKE_CONFIG_DIR etc/osmanov/beectl)
endif(NOT CMAKE_CONFIG_DIR)
install(FILES LICENSE README.md DESTINATION "${CMAKE_DOC_DIR}" COMPONENT config)

# Install manifests

string(TOUPPER "${CMAKE_SYSTEM_NAME}" uppercase_CMAKE_SYSTEM_NAME)
if(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)
    string(REPLACE "/" "\\\\" BEECTL_NSIS_CHROME_MANIFEST_DIR
        "${CMAKE_INSTALL_PREFIX}\\\\${BEECTL_CHROME_MANIFEST_DIR}")
    string(REPLACE "/" "\\\\" BEECTL_NSIS_FIREFOX_MANIFEST_DIR
        "${CMAKE_INSTALL_PREFIX}\\\\${BEECTL_FIREFOX_MANIFEST_DIR}")

    # Install host app.
    #
    # On Windows, manifest "path" property is relative to the directory where
    # the manifest is located. So we install the app to the same directories as
    # the manifest files for our convenience. Otherwise, for absolute path, we
    # would have to patch the manifest file during installation, since the
    # Windows installer provides the choice of the destination directory.
    set(BEECTL_MANIFEST_TARGET_PATH "beectl.exe")
    install(TARGETS beectl RUNTIME DESTINATION "${BEECTL_CHROME_MANIFEST_DIR}" COMPONENT applications)
    install(TARGETS beectl RUNTIME DESTINATION "${BEECTL_FIREFOX_MANIFEST_DIR}" COMPONENT applications)

    # NSIS package properties
    #set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake/modules/nsis" ${CMAKE_MODULE_PATH})
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/beectl.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${CPACK_NSIS_MUI_ICON}")
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_DESCRIPTION}")
    set(CPACK_NSIS_HELP_LINK "https://github.com/rosmanov/chrome-bee")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/rosmanov/chrome-bee")
    set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKCU 'Software\\\\${PROJECT_NAME}' 'Install Directory' '$INSTDIR'
        WriteRegStr HKCU 'Software\\\\Google\\\\Chrome\\\\NativeMessagingHosts\\\\com.ruslan_osmanov.bee' '' '$INSTDIR${BEECTL_NSIS_CHROME_MANIFEST_DIR}\\\\com.ruslan_osmanov.bee.json'
        WriteRegStr HKCU 'Software\\\\Mozilla\\\\NativeMessagingHosts\\\\com.ruslan_osmanov.bee' '' '$INSTDIR${BEECTL_NSIS_FIREFOX_MANIFEST_DIR}\\\\com.ruslan_osmanov.bee.json'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKCU 'Software\\\\${PROJECT_NAME}'
        DeleteRegKey HKCU 'Software\\\\Google\\\\Chrome\\\\NativeMessagingHosts'
        DeleteRegKey HKCU 'Software\\\\Mozilla\\\\NativeMessagingHosts'
    ")
    #configure_file(
    #${PROJECT_SOURCE_DIR}/CMake/modules/nsis/NSIS.definitions.nsh.in
    #NSIS.definitions.nsh
    #)

    configure_file(
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        firefox-com.ruslan_osmanov.bee.json.in
        "${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_CHROME_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_FIREFOX_MANIFEST_DIR}"
        COMPONENT config)
elseif(uppercase_CMAKE_SYSTEM_NAME MATCHES DARWIN)
    set(BEECTL_CHROME_MANIFEST_DIR "Library/Google/Chrome/NativeMessagingHosts")
    set(BEECTL_FIREFOX_MANIFEST_DIR  "Library/Application Support/Mozilla/NativeMessagingHosts")

    # Install host app
    install(TARGETS beectl RUNTIME DESTINATION bin COMPONENT applications)

    # productbuild doesn't accept doc files without extensions
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        @ONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

    configure_file(
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        firefox-com.ruslan_osmanov.bee.json.in
        "${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "/${BEECTL_CHROME_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "/${BEECTL_FIREFOX_MANIFEST_DIR}"
        COMPONENT config)
else(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)
    # Install host app
    install(TARGETS beectl RUNTIME DESTINATION bin COMPONENT applications)

    configure_file(
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        firefox-com.ruslan_osmanov.bee.json.in
        "${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "/${BEECTL_CHROME_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "/${BEECTL_FIREFOX_MANIFEST_DIR}"
        COMPONENT config)
endif(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)


include(CPack)
