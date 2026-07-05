cmake_minimum_required(VERSION 3.12.0...4.0.1)
option(CPACK_GENERATOR "CPack generator" "RPM DEB TGZ ZIP productbuild")

project(BeeCtl
  VERSION 1.5.0
  DESCRIPTION "Native Messaging Host for the Bee Browser Extension <https://github.com/rosmanov/chrome-bee>"
  HOMEPAGE_URL "https://github.com/rosmanov/bee-host"
  LANGUAGES C)
set(PACKAGE_RELEASE 1) # Increment this only when the package is changed, not the code of the app.

set(PROJECT_CONTACT "Ruslan Osmanov <608192+rosmanov@users.noreply.github.com>")
set(PROJECT_LICENSE "MIT")

# Generate Nix flake from template
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/flake.nix.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/flake.nix"
  @ONLY
)

# Generate Arch Linux PKGBUILD from template
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/arch/PKGBUILD.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/arch/PKGBUILD"
  @ONLY
)

string(TOUPPER "${CMAKE_SYSTEM_NAME}" uppercase_CMAKE_SYSTEM_NAME)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static library build")

# C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Option to use system-provided dependencies (for Nix, distro packages, etc.)
option(USE_SYSTEM_DEPS "Use system-provided libuv and cJSON instead of downloading" OFF)

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

# ============================================================================
# Dependency Setup Functions
# ============================================================================

# Function: setup_system_dependencies
# Finds and configures system-provided libuv and cJSON (USE_SYSTEM_DEPS=ON)
# Prefers static libraries, falls back to dynamic if unavailable
# Sets: BEECTL_LIBUV_LIBRARIES, BEECTL_CJSON_LIBRARIES,
#       BEECTL_LIBUV_INCLUDE_DIRS, BEECTL_CJSON_INCLUDE_DIRS,
#       BEECTL_EXTERNAL_TARGETS (empty)
function(setup_system_dependencies)
  message(STATUS "Using system-provided libuv and cJSON")

  find_package(PkgConfig REQUIRED)

  # Find libuv and cJSON via pkg-config
  pkg_check_modules(LIBUV REQUIRED libuv>=1.51.0)
  pkg_check_modules(CJSON REQUIRED libcjson>=1.7.18)

  # Prefer static libraries if available
  find_library(LIBUV_STATIC_LIB
    NAMES libuv.a libuv_a uv.lib
    HINTS ${LIBUV_LIBRARY_DIRS}
    NO_DEFAULT_PATH
  )
  find_library(CJSON_STATIC_LIB
    NAMES libcjson.a libcjson_a cjson.lib
    HINTS ${CJSON_LIBRARY_DIRS}
    NO_DEFAULT_PATH
  )

  # Set library variables (static if found, otherwise dynamic)
  if(LIBUV_STATIC_LIB)
    message(STATUS "Using static libuv: ${LIBUV_STATIC_LIB}")
    set(BEECTL_LIBUV_LIBRARIES "${LIBUV_STATIC_LIB}" PARENT_SCOPE)
  else()
    message(STATUS "Using dynamic libuv: ${LIBUV_LIBRARIES}")
    set(BEECTL_LIBUV_LIBRARIES "${LIBUV_LIBRARIES}" PARENT_SCOPE)
  endif()

  if(CJSON_STATIC_LIB)
    message(STATUS "Using static cJSON: ${CJSON_STATIC_LIB}")
    set(BEECTL_CJSON_LIBRARIES "${CJSON_STATIC_LIB}" PARENT_SCOPE)
  else()
    message(STATUS "Using dynamic cJSON: ${CJSON_LIBRARIES}")
    set(BEECTL_CJSON_LIBRARIES "${CJSON_LIBRARIES}" PARENT_SCOPE)
  endif()

  # Set include directories
  set(BEECTL_LIBUV_INCLUDE_DIRS "${LIBUV_INCLUDE_DIRS}" PARENT_SCOPE)
  set(BEECTL_CJSON_INCLUDE_DIRS "${CJSON_INCLUDE_DIRS}" PARENT_SCOPE)

  # No external project targets
  set(BEECTL_EXTERNAL_TARGETS "" PARENT_SCOPE)
endfunction()

# Function: setup_external_dependencies
# Downloads and builds libuv and cJSON from GitHub (USE_SYSTEM_DEPS=OFF)
# Builds as static libraries using ExternalProject_Add
# Sets: BEECTL_LIBUV_LIBRARIES, BEECTL_CJSON_LIBRARIES,
#       BEECTL_LIBUV_INCLUDE_DIRS, BEECTL_CJSON_INCLUDE_DIRS,
#       BEECTL_EXTERNAL_TARGETS (list of targets)
function(setup_external_dependencies)
  message(STATUS "Downloading and building libuv and cJSON from source")

  # Configure cJSON build
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
  set(CJSON_STATIC_LIB "${CJSON_BINARY_DIR}/${CJSON_STATIC_NAME}")

  # Configure libuv build
  set(LIBUV_TAG v1.51.0)
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
  set(LIBUV_STATIC_LIB "${LIBUV_BINARY_DIR}/${LIBUV_STATIC_NAME}")

  # Set standardized output variables for parent scope
  set(BEECTL_LIBUV_LIBRARIES "${LIBUV_STATIC_LIB}" PARENT_SCOPE)
  set(BEECTL_CJSON_LIBRARIES "${CJSON_STATIC_LIB}" PARENT_SCOPE)
  set(BEECTL_LIBUV_INCLUDE_DIRS "${LIBUV_SOURCE_DIR}/include" PARENT_SCOPE)
  set(BEECTL_CJSON_INCLUDE_DIRS "${CJSON_DOWNLOAD_DIR}" PARENT_SCOPE)
  set(BEECTL_EXTERNAL_TARGETS "cjson;libuv_ep" PARENT_SCOPE)
endfunction()

# ============================================================================
# Main Dependency Setup
# ============================================================================

if(USE_SYSTEM_DEPS)
  setup_system_dependencies()
else()
  setup_external_dependencies()
endif()

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

# Include directories (using standardized variables from setup functions)
target_include_directories(beectl PRIVATE
  ${BEECTL_CJSON_INCLUDE_DIRS}
  ${BEECTL_LIBUV_INCLUDE_DIRS}
)

target_compile_definitions(beectl PRIVATE
  PROJECT_VERSION="${PROJECT_VERSION}-${PACKAGE_RELEASE}"
  PROJECT_DESCRIPTION="${PROJECT_DESCRIPTION}"
  PROJECT_COPYRIGHT="\(C\) 2019-2025 ${PROJECT_CONTACT}"
  PROJECT_LICENSE="${PROJECT_LICENSE}"
)

# Link libraries (using standardized variables from setup functions)
target_link_libraries(beectl PRIVATE
  ${BEECTL_LIBUV_LIBRARIES}
  ${BEECTL_CJSON_LIBRARIES}
)

# Platform-specific system libraries
if(WIN32)
  # For libuv on Windows/MinGW
  target_link_libraries(beectl PRIVATE ws2_32 iphlpapi dbghelp userenv ssp)
endif()

if(UNIX AND NOT APPLE)
  # libuv requires pthread and dl on Linux
  target_link_libraries(beectl PRIVATE pthread dl)
endif()

set_property(TARGET beectl PROPERTY C_STANDARD 11) # C11

# Add external project dependencies if any (empty for system deps)
if(BEECTL_EXTERNAL_TARGETS)
  add_dependencies(beectl ${BEECTL_EXTERNAL_TARGETS})
endif()

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

include(GNUInstallDirs)

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

# Determine if we are installing to a standard system prefix
if(CMAKE_INSTALL_PREFIX STREQUAL "/usr" OR CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" OR CMAKE_INSTALL_PREFIX STREQUAL "/")
    set(BEECTL_STANDARD_PREFIX ON)
else()
    set(BEECTL_STANDARD_PREFIX OFF)
endif()

set(BEECTL_BIN_DIR "${CMAKE_INSTALL_BINDIR}" CACHE STRING "Install directory for the beectl executable")
set(BEECTL_MANIFEST_TARGET_PATH "${CMAKE_INSTALL_PREFIX}/${BEECTL_BIN_DIR}/beectl" CACHE STRING "Path to beectl executable in the manifest files")

if(uppercase_CMAKE_SYSTEM_NAME MATCHES DARWIN)
    set(default_chrome_manifest_dir "Library/Google/Chrome/NativeMessagingHosts")
    set(default_chromium_manifest_dir "Library/Application Support/Chromium/NativeMessagingHosts")
    set(default_firefox_manifest_dir "Library/Application Support/Mozilla/NativeMessagingHosts")
    set(default_doc_dir "Library/Application Support/beectl")
else()
    # On Linux/UNIX, use absolute system-wide paths if installing to a standard prefix.
    # Otherwise, use relative paths to support custom installation sandboxes and relocatable TGZ packaging.
    if(BEECTL_STANDARD_PREFIX)
        set(default_chrome_manifest_dir "/etc/opt/chrome/native-messaging-hosts")
        set(default_chromium_manifest_dir "/etc/chromium/native-messaging-hosts")
        set(default_firefox_manifest_dir "/usr/lib/mozilla/native-messaging-hosts")
        set(default_firefox_manifest_dir_alt "/usr/lib64/mozilla/native-messaging-hosts")
        
        if(CMAKE_INSTALL_PREFIX STREQUAL "/")
            set(default_doc_dir "usr/share/doc/osmanov/beectl")
        else()
            set(default_doc_dir "share/doc/osmanov/beectl")
        endif()
    else()
        set(default_chrome_manifest_dir "etc/opt/chrome/native-messaging-hosts")
        set(default_chromium_manifest_dir "etc/chromium/native-messaging-hosts")
        set(default_firefox_manifest_dir "usr/lib/mozilla/native-messaging-hosts")
        set(default_firefox_manifest_dir_alt "usr/lib64/mozilla/native-messaging-hosts")
        set(default_doc_dir "share/doc/osmanov/beectl")
    endif()
endif()

set(BEECTL_CHROME_MANIFEST_DIR "${default_chrome_manifest_dir}" CACHE STRING "Chrome manifest install directory")
set(BEECTL_CHROMIUM_MANIFEST_DIR "${default_chromium_manifest_dir}" CACHE STRING "Chromium manifest install directory")
set(BEECTL_FIREFOX_MANIFEST_DIR "${default_firefox_manifest_dir}" CACHE STRING "Firefox manifest install directory")

if(UNIX AND NOT APPLE)
    # Fedora uses /usr/lib64 while Pop OS uses /usr/lib for amd64. So we install both.
    set(BEECTL_FIREFOX_MANIFEST_DIR_ALT "${default_firefox_manifest_dir_alt}" CACHE STRING "Alternative Firefox manifest install directory")
endif()

if(NOT CMAKE_DOC_DIR)
    set(CMAKE_DOC_DIR "${default_doc_dir}" CACHE STRING "Installation directory for documentation")
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
set(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_PACKAGE_RELEASE}")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")

# Install docs
if(NOT uppercase_CMAKE_SYSTEM_NAME MATCHES DARWIN)
    list(APPEND CPACK_RPM_RELOCATION_PATHS "${CMAKE_DOC_DIR}")
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
elseif(uppercase_CMAKE_SYSTEM_NAME MATCHES DARWIN)
    # Install host app
    install(TARGETS beectl RUNTIME DESTINATION "${BEECTL_BIN_DIR}" COMPONENT applications)

    # productbuild doesn't accept doc files without extensions
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        @ONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
else(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)
    # Install host app
    install(TARGETS beectl RUNTIME DESTINATION "${BEECTL_BIN_DIR}" COMPONENT applications)
endif(uppercase_CMAKE_SYSTEM_NAME MATCHES WINDOWS)

# Generate and install browser manifest files globally
foreach(browser chrome chromium firefox)
    if(browser STREQUAL "firefox")
        set(template "firefox-com.ruslan_osmanov.bee.json.in")
        set(dest "${BEECTL_FIREFOX_MANIFEST_DIR}")
    elseif(browser STREQUAL "chrome")
        set(template "chrome-com.ruslan_osmanov.bee.json.in")
        set(dest "${BEECTL_CHROME_MANIFEST_DIR}")
    else()
        set(template "chrome-com.ruslan_osmanov.bee.json.in")
        set(dest "${BEECTL_CHROMIUM_MANIFEST_DIR}")
    endif()

    configure_file(
        "${template}"
        "manifests_build/${browser}/com.ruslan_osmanov.bee.json"
        @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/manifests_build/${browser}/com.ruslan_osmanov.bee.json"
        DESTINATION "${dest}"
        COMPONENT config)
endforeach()

# Install manifest to an alternative location to address the issue of different conventions
# on the /usr/lib and /usr/lib64 directories across Linux distros (Fedora uses /usr/lib64,
# Pop OS uses /usr/lib, Arch uses /usr/lib etc.)
if(BEECTL_FIREFOX_MANIFEST_DIR_ALT)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/manifests_build/firefox/com.ruslan_osmanov.bee.json"
        DESTINATION "${BEECTL_FIREFOX_MANIFEST_DIR_ALT}"
        COMPONENT config)
endif()

set(CPACK_SET_DESTDIR ON)
include(CPack)
