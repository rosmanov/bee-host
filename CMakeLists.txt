cmake_minimum_required(VERSION 3.12.0...4.0.1)
option(CPACK_GENERATOR "CPack generator" "RPM DEB TGZ ZIP productbuild")

project(BeeCtl
  VERSION 1.4.2
  DESCRIPTION "Native Messaging Host for the Bee Browser Extension <https://github.com/rosmanov/chrome-bee>"
  HOMEPAGE_URL "https://github.com/rosmanov/bee-host"
  LANGUAGES C)
set(PACKAGE_RELEASE 1) # Increment this only when the package is changed, not the code of the app.

set(PROJECT_CONTACT "Ruslan Osmanov <608192+rosmanov@users.noreply.github.com>")
set(PROJECT_LICENSE "MIT")

string(TOUPPER "${CMAKE_SYSTEM_NAME}" uppercase_CMAKE_SYSTEM_NAME)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static library build")

# C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

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
check_function_exists(mkstemps HAVE_MKSTEMPS)
if(HAVE_MKSTEMPS)
  add_definitions(-DHAVE_MKSTEMPS)
endif()
check_function_exists(basename HAVE_BASENAME)
if(HAVE_BASENAME)
  add_definitions(-DHAVE_BASENAME)
endif()

set(EXTERNAL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)

set(cjson_cmake_args
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DBUILD_SHARED_AND_STATIC_LIBS=On
    -DBUILD_SHARED_LIBS=Off
    -DENABLE_CJSON_TEST=Off
    -DENABLE_CJSON_UTILS=On
    -DENABLE_TARGET_EXPORT=On
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}
    -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}
    -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
)
if(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)
    list(APPEND cjson_cmake_args -DWIN32=ON)
endif()

ExternalProject_Add(cjson
  GIT_REPOSITORY "https://github.com/DaveGamble/cJSON"
  GIT_TAG "v1.7.18"
  PREFIX "${EXTERNAL_DIR}/cJSON"
  BUILD_IN_SOURCE 1
  TEST_COMMAND ""
  INSTALL_COMMAND ""
  CMAKE_ARGS ${cjson_cmake_args}
)
ExternalProject_Get_Property(cjson source_dir)
ExternalProject_Get_Property(cjson binary_dir)
ExternalProject_Get_Property(cjson download_dir)
set(CJSON_SOURCE_DIR "${source_dir}")
set(CJSON_BINARY_DIR "${binary_dir}")
set(CJSON_DOWNLOAD_DIR "${download_dir}")
if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CJSON_STATIC_NAME "cjson.lib")
else()
    set(CJSON_STATIC_NAME "libcjson.a")
endif()
# Full path to the expected built static lib
set(CJSON_STATIC_LIB "${CJSON_BINARY_DIR}/${CJSON_STATIC_NAME}")

set(LIBUV_TAG v1.51.0)
set(EXTERNAL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)

set(libuv_cmake_args
  -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
  -DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
  -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
  -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}
  -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  -DLIBUV_BUILD_TESTS=OFF
  -DLIBUV_BUILD_EXAMPLES=OFF
  -DLIBUV_INSTALL=OFF
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
)
# Append UV_DISABLE_CLOCK_GETTIME only for Windows
if(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)
  list(APPEND libuv_cmake_args -DWIN32=ON)
endif()

ExternalProject_Add(libuv_ep
  GIT_REPOSITORY "https://github.com/libuv/libuv.git"
  GIT_TAG "${LIBUV_TAG}"
  PREFIX "${EXTERNAL_DIR}/libuv"
  CMAKE_ARGS ${libuv_cmake_args}
  INSTALL_COMMAND ""
  TEST_COMMAND ""
)
ExternalProject_Get_Property(libuv_ep source_dir)
ExternalProject_Get_Property(libuv_ep binary_dir)
set(LIBUV_SOURCE_DIR "${source_dir}")
set(LIBUV_BINARY_DIR "${binary_dir}")
if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(LIBUV_STATIC_NAME "uv.lib")
else()
    set(LIBUV_STATIC_NAME "libuv.a")
endif()
# Full path to the expected built static lib
set(LIBUV_STATIC_LIB "${LIBUV_BINARY_DIR}/${LIBUV_STATIC_NAME}")

# beectl
set(BEECTL_SRCS
  src/beectl.c
  src/str.c
  src/io.c
  src/mkstemps.c
  src/basename.c
  # This is nasty, but I couldn't find a way to use CMAKE_TOOLCHAIN_FILE
  # for the external project (see comments below.)
  #"${CJSON_SOURCE_DIR}/cJSON.c"
  )

add_executable(beectl ${BEECTL_SRCS})

# Workaround for CMake versions which require the cJSON.c file to exist before
# downloading the external project.
#add_custom_command(TARGET beectl PRE_BUILD
    #BYPRODUCTS "${CJSON_SOURCE_DIR}/cJSON.c"
    #COMMAND touch "${CJSON_SOURCE_DIR}/cJSON.c")
if(APPLE)
add_custom_command(TARGET beectl POST_BUILD
    COMMAND strip -x $<TARGET_FILE:beectl>
    COMMENT "Stripping local symbols to reduce binary size"
)
endif(APPLE)

target_compile_features(beectl PRIVATE c_variadic_macros)
target_compile_options(beectl PRIVATE
  "$<$<CONFIG:Release>:-Os;-ffunction-sections;-fdata-sections;-DNDEBUG>"
)
target_include_directories(beectl PRIVATE "${CJSON_DOWNLOAD_DIR}")
target_include_directories(beectl PRIVATE "${libuv_SOURCE_DIR}/include")

target_compile_definitions(beectl PRIVATE
  PROJECT_VERSION="${PROJECT_VERSION}-${PACKAGE_RELEASE}"
  PROJECT_DESCRIPTION="${PROJECT_DESCRIPTION}"
  PROJECT_COPYRIGHT="\(C\) 2019-2025 ${PROJECT_CONTACT}"
  PROJECT_LICENSE="${PROJECT_LICENSE}"
)

target_link_libraries(beectl PRIVATE "${LIBUV_STATIC_LIB}" "${CJSON_STATIC_LIB}")
#target_link_libraries(beectl PRIVATE "${LIBUV_STATIC_LIB}")
if(WIN32)
    # For libuv on Windows/MinGW
    target_link_libraries(beectl PRIVATE ws2_32 iphlpapi dbghelp userenv ssp)
endif()
target_include_directories(beectl PRIVATE "${LIBUV_SOURCE_DIR}/include")

if(UNIX AND NOT APPLE)
  # Since we are bypassing libuv's CMake step by using ExternalProject_Add() instead of
  # FetchContent_Declare(), we must link pthread and dl manually as libuv requires them.
  # Note that we don't use FetchContent because it automatically installs extra files that
  # eventually appear in the CPack-generated packages. By using ExternalProject, we avoid
  # the install step.
  target_link_libraries(beectl PRIVATE pthread dl)
endif()

set_property(TARGET beectl PROPERTY C_STANDARD 11) # C11
add_dependencies(beectl cjson)
add_dependencies(beectl libuv_ep)

# The following would be better approach than including cJSON files in our
# source list. But the build fails when cross-compiling, since
# ExternalProject_Add() does not regard the CMAKE_TOOLCHAIN_FILE. Passing
# CMAKE_TOOLCHAIN_FILE via CMAKE_ARGS didn't work either.
#target_link_libraries(beectl
    #"${CJSON_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}cjson${CMAKE_STATIC_LIBRARY_SUFFIX}")


# CPack common properties
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_NAME "beectl")
set(CPACK_PACKAGE_RELEASE ${PACKAGE_RELEASE})
set(CPACK_PACKAGE_CONTACT "${PROJECT_CONTACT}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${PROJECT_NAME}")
set(OLD_CPACK_PACKAGING_INSTALL_PREFIX "${CPACK_PACKAGING_INSTALL_PREFIX}")
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_FILE_NAME
  "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.${CMAKE_BUILD_TYPE}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/\\\\.build-id/"
    "/CMake/"
    "/CMakeCache\\\\.txt.*"
    "/CMakeFiles/"
    "/_CPack_Packages/"
    "\\\\.cmake$"
    "/\\\\.(git|svn)(ignore)?/"
    "~$"
    "/\\\\..*/"
    "cscope.*"
    "/external/.*/(examples|tests|docs)/"
    "\\\\.sw[po]$"
    "\\\\.(gz|Z|xz|zip|log|deb|rpm|pkg|bz2|exe|msi)$"
)
# Limit packaging to only the specified components.
set(CPACK_COMPONENTS_ALL applications config)

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
set(BEECTL_MANIFEST_TARGET_PATH "${CMAKE_INSTALL_PREFIX}/bin/beectl")
set(BEECTL_CHROME_MANIFEST_DIR "etc/opt/chrome/native-messaging-hosts")
set(BEECTL_CHROMIUM_MANIFEST_DIR "etc/chromium/native-messaging-hosts")

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    set(BEECTL_FIREFOX_MANIFEST_DIR "usr/lib/mozilla/native-messaging-hosts")
    # Fedora uses /usr/lib64 while Pop OS uses /usr/lib for amd64. So we install both.
    set(BEECTL_FIREFOX_MANIFEST_DIR_ALT "usr/lib64/mozilla/native-messaging-hosts")
else()
    set(BEECTL_FIREFOX_MANIFEST_DIR "usr/lib/mozilla/native-messaging-hosts")
endif()

# RPM package properties
set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/changelog")
set(CPACK_RPM_PACKAGE_GROUP "Development Tools")
set(CPACK_RPM_PACKAGE_LICENSE "${PROJECT_LICENSE}")
set(CPACK_RPM_BUILDREQUIRES "git, cmake >= 3.12, make, gcc")
set(CPACK_RPM_PACKAGE_RELEASE "${CPACK_PACKAGE_RELEASE}")
set(
    CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr
    /usr/bin
    /usr/local
    /usr/local/bin
    /usr/lib
    /usr/lib/.build-id
    "${CMAKE_INSTALL_BINDIR}"
    "/${BEECTL_CHROME_MANIFEST_DIR}"
    "/${BEECTL_CHROMIUM_MANIFEST_DIR}"
    "/${BEECTL_FIREFOX_MANIFEST_DIR}"
)
# Disable automatic build-id section in RPMs
set(CPACK_RPM_SPEC_MORE_DEFINE "%global _build_id_links none")
# Prevent rpmbuild from calling the native x86_64 'strip' tool on the arm/ppc64* binary
# (we are cross-compiling on an x86_64 Linux host).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CPACK_RPM_SPEC_MORE_DEFINE "%define __strip arm-linux-gnueabihf-strip")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64le")
    set(CPACK_RPM_SPEC_MORE_DEFINE "%define __strip powerpc64le-linux-gnu-strip")
  endif()
endif()

list(APPEND CPACK_RPM_RELOCATION_PATHS
    "${CMAKE_INSTALL_BINDIR}"
    "/${BEECTL_CHROME_MANIFEST_DIR}"
    "/${BEECTL_CHROMIUM_MANIFEST_DIR}"
    "/${BEECTL_FIREFOX_MANIFEST_DIR}"
)

set(CPACK_PACKAGING_INSTALL_PREFIX "${OLD_CPACK_PACKAGING_INSTALL_PREFIX}")

# DEB package properties
if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_DEBIAN_PACKAGE_RELEASE, "${CPACK_PACKAGE_RELEASE}")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")

# Install docs
if(uppercase_CMAKE_SYSTEM_NAME MATCHES DARWIN)
    if(NOT CMAKE_DOC_DIR)
        set(CMAKE_DOC_DIR "Library/Application Support/beectl")
    endif(NOT CMAKE_DOC_DIR)
else()
    if(NOT CMAKE_DOC_DIR)
        set(CMAKE_DOC_DIR usr/share/doc/osmanov/beectl)
        list(APPEND CPACK_RPM_RELOCATION_PATHS "${CMAKE_DOC_DIR}")
    endif(NOT CMAKE_DOC_DIR)
endif()
install(FILES LICENSE README.md DESTINATION "${CMAKE_DOC_DIR}" COMPONENT config)

# Install manifests

if(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)
    string(REPLACE "/" "\\\\" BEECTL_NSIS_CHROME_MANIFEST_DIR
        "${BEECTL_CHROME_MANIFEST_DIR}")
    string(REPLACE "/" "\\\\" BEECTL_NSIS_FIREFOX_MANIFEST_DIR
        "${BEECTL_FIREFOX_MANIFEST_DIR}")

    # Install host app.
    #
    # On Windows, manifest "path" property is relative to the directory where
    # the manifest is located. So we install the app to the same directories as
    # the manifest files for our convenience. Otherwise, for absolute path, we
    # would have to patch the manifest file during installation, since the
    # Windows installer provides the choice of the destination directory.
    set(BEECTL_MANIFEST_TARGET_PATH "beectl.exe")
    install(TARGETS beectl RUNTIME DESTINATION "${BEECTL_CHROME_MANIFEST_DIR}" COMPONENT applications)
    install(TARGETS beectl RUNTIME DESTINATION "${BEECTL_CHROMIUM_MANIFEST_DIR}" COMPONENT applications)
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
        WriteRegStr HKCU 'Software\\\\Google\\\\Chrome\\\\NativeMessagingHosts\\\\com.ruslan_osmanov.bee' '' '$INSTDIR\\\\${BEECTL_NSIS_CHROME_MANIFEST_DIR}\\\\com.ruslan_osmanov.bee.json'
        WriteRegStr HKCU 'Software\\\\Mozilla\\\\NativeMessagingHosts\\\\com.ruslan_osmanov.bee' '' '$INSTDIR\\\\${BEECTL_NSIS_FIREFOX_MANIFEST_DIR}\\\\com.ruslan_osmanov.bee.json'
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
        "${CMAKE_CURRENT_SOURCE_DIR}/beectl.wxs.in"
        "${CMAKE_CURRENT_BINARY_DIR}/beectl.wxs"
        @ONLY
    )
    configure_file(
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROMIUM_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        firefox-com.ruslan_osmanov.bee.json.in
        "${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_CHROME_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROMIUM_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_CHROMIUM_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_FIREFOX_MANIFEST_DIR}"
        COMPONENT config)
elseif(uppercase_CMAKE_SYSTEM_NAME MATCHES DARWIN)
    set(BEECTL_CHROME_MANIFEST_DIR "Library/Google/Chrome/NativeMessagingHosts")
    set(BEECTL_CHROMIUM_MANIFEST_DIR "Library/Application Support/Chromium/NativeMessagingHosts")
    set(BEECTL_FIREFOX_MANIFEST_DIR  "Library/Application Support/Mozilla/NativeMessagingHosts")

    # Install host app
    install(TARGETS beectl RUNTIME DESTINATION usr/local/bin COMPONENT applications)

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
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROMIUM_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        firefox-com.ruslan_osmanov.bee.json.in
        "${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_CHROME_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROMIUM_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_CHROMIUM_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_FIREFOX_MANIFEST_DIR}"
        COMPONENT config)
else(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)
    # Install host app
    install(TARGETS beectl RUNTIME DESTINATION usr/local/bin COMPONENT applications)

    configure_file(
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        chrome-com.ruslan_osmanov.bee.json.in
        "${BEECTL_CHROMIUM_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)
    configure_file(
        firefox-com.ruslan_osmanov.bee.json.in
        "${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROME_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_CHROME_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_CHROMIUM_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_CHROMIUM_MANIFEST_DIR}"
        COMPONENT config)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_FIREFOX_MANIFEST_DIR}"
        COMPONENT config)
endif(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)

# Install manifest to an alternative location to address the issue of different conventions
# on the /usr/lib and /usr/lib64 directories across Linux distros (Fedora uses /usr/lib64,
# Pop OS uses /usr/lib, Arch uses /usr/lib etc.)
if(BEECTL_FIREFOX_MANIFEST_DIR_ALT)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${BEECTL_FIREFOX_MANIFEST_DIR}/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_FIREFOX_MANIFEST_DIR_ALT}"
        COMPONENT config)
endif()

include(CPack)
