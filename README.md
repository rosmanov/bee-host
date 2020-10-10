# About

A native messaging host application for [Browser's Exernal Editor extension](https://github.com/rosmanov/chrome-bee).

## Supported Operating Systems

- GNU/Linux
- Windows (MinGW binaries*)
- macOS (tested on 10.15.6)

# Installing

Precompiled binaries can be downloaded from [SourceForge](https://sourceforge.net/projects/beectl/).

## RPM

```
rpm -Uvh --nodeps beectl-$VERSION.$ARCH.$RELEASE.rpm
```
where `$VERSION` is the package and release version, `$ARCH` is the architecture name, e.g.

```
rpm -Uvh --nodeps beectl-1.0.0-1.amd64.Release.rpm
```

## DEB

```
dpkg -i beectl-$VERSION.$ARCH.$RELEASE.deb
```

e.g.

```
dpkg -i beectl-1.0.0-1.amd64.Release.deb
```

# Uninstalling

## RPM

```
rpm -e beectl-$VERSION.$ARCH.rpm
```
where `$VERSION` is the package and release version, `$ARCH` is the architecture name.

## DEB

Using apt:
```
apt purge beectl
```

# Building Manually

Build system is based on CMake toolchains (`CMake/Toolchain-*.cmake`) using *GCC* compiler for GNU/Linux and a *MinGW* port of GCC for Windows. The host is supposed to be a GNU/Linux system.

## 64-bit GNU/Linux (amd64)

```
./build-linux-amd64.sh -b Release
```

## 32-bit GNU/Linux (i386)

```
./build-linux-i386.sh -b Release
```

## 64-bit GNU/Linux (amd64)

```
./build-linux-amd64.sh -b Release
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

After building the project, run `make package`. The command should run CPack with a generator matching the current CMake toolchain (e.g. RPM for GNU/Linux, NSIS for Windows etc.) As a result, a package should be generated in the project root.
