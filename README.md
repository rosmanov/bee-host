# About

A native messaging host application for [Browser's Exernal Editor extension](https://github.com/rosmanov/chrome-bee).

## Supported Operating Systems

- GNU/Linux
- Windows

The code should work just fine on macOS, but I haven't yet tested it there.

# Building

The build system is based on CMake toolchains (`CMake/Toolchain-*.cmake`) using *GCC* compiler for GNU/Linux and a *MinGW* port of GCC for Windows. So the host is supposed to be a GNU/Linux system.

## 64-bit GNU/Linux (amd64)

```
./build-linux-amd64.sh -b Release
```

## 32-bit GNU/Linux (i386)

```
./build-linux-i386.sh -b Release
```

## 32-bit Windows (i686)

```
./build-win-i686.sh -b Release
```

## Other CPU Architectures

Build scripts/toolchains for other CPU architectures can be added upon request.

Path to a custom toolchain can be passed to `build.sh` script as follows:

```
./build.sh /path/to/custom-toolchain.cmake -b Release
```

## Debug Version

`./build.sh` builds debug version by default (if the build type is not specified with `-b` option). Build type can also be passed explicitly using `-b` option, e.g.:

```
./build.sh all -b Debug
```

(The command above iterates through `all` `Toolchain-*.cmake` toolchains in the `CMake` directory.)

# Packaging

After building the project, run `make package`. The command should run CPack with a generator matching the current CMake toolchain (e.g. RPM for GNU/Linux, NSIS for Windows etc.)
