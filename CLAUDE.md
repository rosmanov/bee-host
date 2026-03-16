# BeeCtl - Native Messaging Host

## Project Overview

BeeCtl is a native messaging host application for the [Browser's External Editor (Bee) extension](https://github.com/rosmanov/chrome-bee). It enables communication between web browsers (Chrome, Firefox, Chromium) and external text editors, allowing users to edit text areas in their preferred editor.

**Key Functionality:**
- Receives text content from browser extensions via native messaging protocol
- Launches external text editors with the content
- Monitors file changes using libuv's file system events
- Sends updated content back to the browser

**Supported Platforms:**
- Linux (amd64, i386, arm, aarch64, ppc64le)
- Windows (amd64, i686) - cross-compiled using MinGW
- macOS (x86_64, arm64)
- FreeBSD

---

## Architecture

### Core Technologies
- **Language:** C11
- **Build System:** CMake 3.12+
- **Event Loop:** libuv v1.51.0 (for file watching and async I/O)
- **JSON Parsing:** cJSON v1.7.18
- **Packaging:** CPack (RPM, DEB, NSIS, TGZ, ZIP, productbuild)

### Project Structure

```
bee-host/
├── src/                    # C source files
│   ├── beectl.c           # Main application logic
│   ├── common.h           # Platform-specific macros and common declarations
│   ├── io.c/io.h          # I/O operations
│   ├── str.c/str.h        # String utilities
│   ├── shell.h            # Shell/process utilities
│   ├── basename.c/h       # Basename utility (for systems without it)
│   ├── mkstemps.c/h       # Secure temp file creation (for systems without it)
│   └── json-patch.c       # JSON patch operations
├── CMake/                  # CMake toolchain files
│   ├── Toolchain-Linux-*.cmake
│   ├── Toolchain-Windows-*.cmake
│   └── Toolchain-macos-*.cmake
├── build/                  # Build output directory (gitignored)
├── build*.sh              # Build scripts for various platforms
├── CMakeLists.txt         # Main CMake configuration
├── flake.nix.in           # Nix flake template (auto-generates flake.nix)
├── flake.nix              # Generated Nix flake (committed to git)
└── README.md              # User documentation
```

---

## Building

### Prerequisites
- CMake 3.12 or higher
- GCC (Linux) or Clang (macOS) or MinGW-GCC (Windows cross-compilation)
- Git (for fetching dependencies)

### Quick Build Commands

**Linux (amd64) - Release:**
```bash
./build-linux-amd64.sh -b Release
```

**macOS (native architecture):**
```bash
./build-macos.sh -b Release
```

**Debug Build:**
```bash
./build.sh -b Debug
```

**Cross-compile for all platforms (requires Docker):**
```bash
./build-cross.sh -b Release
```

**Custom toolchain:**
```bash
./build.sh /path/to/Toolchain-Custom.cmake -b Release
```

### Build System Details

1. **External Dependencies:**
   - **Default mode:** Both libuv and cJSON are fetched automatically via CMake's `ExternalProject_Add()` and built as static libraries
   - **System deps mode:** Set `-DUSE_SYSTEM_DEPS=ON` to use system-provided libuv and cJSON (for Nix, distro packages, etc.)
2. **Static Linking:** The application is statically linked to avoid runtime dependencies (enforced in both modes)
3. **Optimization Flags:** Release builds use `-Os -ffunction-sections -fdata-sections` for size optimization
4. **Binary Stripping:** macOS builds automatically strip local symbols to reduce size

### Creating Packages

```bash
cd build
make package
```

CPack will generate packages appropriate for the platform (RPM, DEB, NSIS installer, etc.).

---

## Coding Conventions

### Style Guidelines

1. **Indentation:** 2 spaces (no tabs)
2. **Naming:**
   - Functions: `snake_case` (e.g., `which()`, `print_help()`)
   - Macros: `SCREAMING_SNAKE_CASE` (e.g., `MAX_PATH`, `DIR_SEPARATOR`)
   - Types: `snake_case_t` for custom types (e.g., `str_t`)
   - Static functions: prefix with `static` and keep them file-local

3. **Comments:**
   - Use `/* */` style comments (C89 compatible)
   - Document complex logic and non-obvious behavior
   - Add file headers with copyright notice

4. **Platform Compatibility:**
   - Use macros from `common.h` for platform-specific code
   - Prefer POSIX functions where available
   - Provide fallbacks for missing functions (e.g., `mkstemps`, `basename`)
   - Use `WINDOWS` macro for Windows-specific code

5. **Memory Management:**
   - Always check allocation results
   - Free allocated memory in reverse order
   - Use `malloc`, `realloc`, `free` (standard C library)

6. **Error Handling:**
   - Check return values of all system calls
   - Use `likely()`/`unlikely()` macros for branch prediction (if available)
   - Print errors to stderr

### Code Patterns

**Platform-specific code example:**
```c
#ifdef WINDOWS
  // Windows-specific code
  setmode(fd, O_BINARY);
#else
  // Unix-specific code
  fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
```

**Using common.h macros:**
```c
DIR_SEPARATOR      // '/' on Unix, '\\' on Windows
PATH_DELIMITER     // ':' on Unix, ';' on Windows
SET_BINARY_MODE(fd) // Handle binary mode for Windows
```

---

## Key Files and Their Roles

### Source Files

- **`src/beectl.c`** - Main entry point, implements native messaging protocol, file watching, and editor launching
- **`src/common.h`** - Platform detection, common macros (`WINDOWS`, `DIR_SEPARATOR`, etc.)
- **`src/io.c/h`** - File I/O operations, reading from stdin, writing to stdout
- **`src/str.c/h`** - String manipulation utilities
- **`src/shell.h`** - Shell command execution
- **`src/basename.c/h`** - Basename implementation for systems without it
- **`src/mkstemps.c/h`** - Secure temporary file creation for systems without it

### Build Configuration

- **`CMakeLists.txt`** - Main build configuration, dependency management, install rules
- **`CMake/Toolchain-*.cmake`** - Cross-compilation toolchains for various platforms
- **`build.sh`** - Generic build script that accepts toolchain path
- **`build-linux-*.sh`** - Linux build shortcuts for specific architectures
- **`build-macos.sh`** - macOS build script
- **`build-cross.sh`** - Docker-based cross-compilation script
- **`helpers.sh`** - Common shell functions used by build scripts

**CMake Options:**

- **`USE_SYSTEM_DEPS`** (default: OFF) - Use system-provided libuv and cJSON instead of downloading from GitHub
  - When OFF: CMake downloads and builds dependencies using `ExternalProject_Add()` (requires network access)
  - When ON: CMake uses pkg-config to find system libraries (sandbox-compatible, used by Nix)
  - Static linking is enforced in both modes when static libraries are available
  - Example: `cmake -DUSE_SYSTEM_DEPS=ON ..`

### Packaging

- **`beectl.spec`** - RPM spec file template
- **`beectl.wxs.in`** - WiX installer XML template (Windows)
- **`changelog`** - RPM changelog
- **`*-com.ruslan_osmanov.bee.json.in`** - Native messaging host manifest templates
- **`flake.nix.in`** - Nix flake template with `@PROJECT_VERSION@` placeholders
- **`flake.nix`** - Generated Nix flake (auto-generated by CMake, committed to git)

---

## Development Workflow

### Making Changes

1. **Edit source files** in `src/`
2. **Build and test:**
   ```bash
   ./build-linux-amd64.sh -b Debug
   ./beectl --help  # Basic sanity check
   ```
3. **Test with browser extension** (requires Bee extension installed)
4. **Run release build** before committing:
   ```bash
   ./build-linux-amd64.sh -b Release
   ```

### Adding New Source Files

1. Add the file to `BEECTL_SRCS` list in `CMakeLists.txt` (lines 135-144)
2. Include appropriate header in `beectl.c` or other source files
3. Ensure compatibility with all target platforms (Windows, Linux, macOS)

### Adding New Platform Support

1. Create a new toolchain file in `CMake/Toolchain-<Platform>-<Arch>.cmake`
2. Define `CMAKE_SYSTEM_NAME`, `CMAKE_SYSTEM_PROCESSOR`, and compiler variables
3. Test with: `./build.sh CMake/Toolchain-<Platform>-<Arch>.cmake -b Release`
4. Add platform-specific install rules in `CMakeLists.txt` if needed

### Cross-Compilation Notes

- **Windows builds** require MinGW toolchain (e.g., `x86_64-w64-mingw32-gcc`)
- **ARM/PowerPC builds** require appropriate cross-compiler (e.g., `arm-linux-gnueabihf-gcc`)
- **Docker-based builds** (`build-cross.sh`) handle toolchain setup automatically
- Toolchains must define both C and C++ compilers (required by some dependencies)

### Nix Flake Support

The project includes a Nix flake for NixOS users and reproducible builds:

- **Template:** `flake.nix.in` contains placeholders (`@PROJECT_VERSION@`, `@PACKAGE_RELEASE@`)
- **Generated file:** `flake.nix` is automatically created by CMake's `configure_file()` command
- **Version source:** CMakeLists.txt is the single source of truth - flake.nix is regenerated on every CMake run
- **Git tracking:** The generated `flake.nix` is committed to git so users can build without running CMake first
- **Sandbox compatible:** Uses `-DUSE_SYSTEM_DEPS=ON` to avoid network access during build (Nix fetches deps before sandbox)
- **Static linking:** Overrides nixpkgs libuv and cJSON to build with static libraries enabled

**Workflow:**
1. Bump version in `CMakeLists.txt`
2. Run `cmake ..` in build directory (regenerates `flake.nix`)
3. Commit both `CMakeLists.txt` and `flake.nix`

**Testing locally:**
```bash
nix build
nix run . -- --help
```

**Testing with sandbox:**
```bash
./test-flake.sh  # Includes sandbox tests
```

**Note:** Nix installs to isolated paths, so users must manually create symlinks for browser manifests (documented in README.md).

---

## Testing

### Manual Testing

1. **Install the native messaging host:**
   ```bash
   cd build
   make install  # or install the generated package
   ```

2. **Install the Bee browser extension** from the Chrome Web Store or Firefox Add-ons

3. **Test editing** a text area on a webpage

### Testing File Watching

Use the provided test script:
```bash
./test-message.py
```

This sends a test message to the native host and verifies the response.

---

## Common Issues and Solutions

### Build Failures

**Issue:** `ExternalProject_Add` fails to download dependencies
- **Solution:** Check network connectivity and Git access to GitHub

**Issue:** Cross-compilation fails with "wrong architecture" errors
- **Solution:** Verify the toolchain file has correct `CMAKE_C_COMPILER` and linker settings

**Issue:** Windows build fails with undefined symbols
- **Solution:** Ensure all required Windows libraries are linked (ws2_32, iphlpapi, dbghelp, userenv, ssp)

### Runtime Issues

**Issue:** Browser can't find the native messaging host
- **Solution:** Check manifest files are installed in correct locations:
  - Linux: `/etc/opt/chrome/native-messaging-hosts/`, `/usr/lib/mozilla/native-messaging-hosts/`
  - macOS: `~/Library/Google/Chrome/NativeMessagingHosts/`, `~/Library/Application Support/Mozilla/NativeMessagingHosts/`
  - Windows: Registry keys under `HKCU\Software\Google\Chrome\NativeMessagingHosts\`

**Issue:** File changes not detected
- **Solution:** This is a known issue (#10), now fixed. Ensure using version 1.4.0+

---

## Version Management

- **Version:** Defined in `CMakeLists.txt` (line 5): `project(BeeCtl VERSION x.y.z ...)`
- **Release Number:** Set in `PACKAGE_RELEASE` variable (line 9)
- Increment **version** for code changes
- Increment **release** only for packaging changes (not code)
- **Single Source of Truth:** CMakeLists.txt is the authoritative version source
  - `flake.nix` is auto-generated from `flake.nix.in` when CMake runs
  - Native messaging manifests use `@PROJECT_VERSION@` and are generated at build time
  - All version numbers propagate from CMakeLists.txt

### Creating a New Release

1. Update version in `CMakeLists.txt` (line 5)
2. Update `changelog` file (for RPM packages)
3. Run CMake to regenerate `flake.nix`: `cd build && cmake ..`
4. Commit version changes: `git add CMakeLists.txt changelog flake.nix`
5. Build packages for all platforms: `./build-cross.sh -b Release`
6. Test packages on target systems
5. Tag release in Git: `git tag -a vX.Y.Z -m "Release X.Y.Z"`
6. Upload packages to GitHub Releases and SourceForge

---

## Important Notes for AI Agents

### When Editing Code

- **Always maintain C11 compatibility** - don't use C++ features or C23 features
- **Test cross-platform** - changes must work on Linux, Windows, and macOS
- **Avoid adding external dependencies** - the project intentionally uses minimal dependencies
- **Preserve static linking** - don't introduce dynamic library dependencies
- **Check pointer arithmetic** - this code manipulates strings and paths extensively
- **Validate before merging** - run at least one platform's build before considering changes complete

### When Debugging

- Debug builds (`-b Debug`) include symbols and disable optimizations
- Use `VERBOSE=1` with make to see full compiler commands: `../build.sh -b Debug VERBOSE=1`
- libuv errors can be checked with `uv_strerror(err)`
- Native messaging protocol uses stdin/stdout with 4-byte length prefix (uint32_t) + JSON message

### When Adding Features

- Consider impact on all platforms (Windows vs Unix differences)
- Update README.md if user-facing features change
- Document new command-line options in `print_help()` function
- Update native messaging manifest templates if protocol changes

### Known Limitations

- Windows builds are cross-compiled on Linux (no native Windows CI)
- ARM64 Windows build not yet supported (requires native build on ARM64 Windows)
- File watching uses debouncing (100ms delay) to coalesce rapid changes
- Temporary files are created in system temp directory (respects `TMPDIR`, `TMP`, `TEMP`)

---

## License

MIT License - see LICENSE file for details.

Copyright © 2019-2025 Ruslan Osmanov
