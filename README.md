# Bee Native Messaging Host

A native messaging host application for the [Browser's External Editor extension](https://github.com/rosmanov/chrome-bee).

## Supported Operating Systems

- Linux
- Windows (MinGW binaries*)
- macOS (tested on 10.15.6 and 15.3.1)
- FreeBSD

> [!NOTE]
> Windows builds are cross-compiled using MinGW.
> Linux builds might also be cross-compiled using Docker.

---

## Installation

### Precompiled Binaries

- Available via [SourceForge](https://sourceforge.net/projects/beectl/) or the [GitHub Releases page](https://github.com/rosmanov/bee-host/releases).
- [FreeBSD Port](https://www.freshports.org/editors/bee-host/)
- RPM packages available via [Copr](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/).

### macOS

Download the latest `.pkg` file from the release links above. Then run the following (replace the filename with your actual download):

```bash
sudo spctl --master-disable
sudo installer -pkg ./beectl-1.3.7-3.x86_64.Release.pkg -target /
sudo spctl --master-enable
```

> [!NOTE]
> Gatekeeper is temporarily disabled due to the lack of an official Apple Developer certificate. This project is not signed with an Apple-issued certificate to avoid the $99/year developer fee.

### RPM (Fedora/Centos/openSUSE)

[![Copr build status](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/package/beectl/status_image/last_build.png?a)](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/package/beectl/)

Enable the repository and install:

```bash
sudo dnf copr enable ruslan-osmanov/beectl
sudo dnf install --refresh beectl
```

Alternatively, manually install the RPM:

```bash
rpm -Uvh --nodeps beectl-<VERSION>.<ARCH>.Release.rpm
```

### DEB (Debian/Ubuntu)

Install the package:

```bash
dpkg -i beectl-<VERSION>.<ARCH>.Release.deb
```

### Windows

For the `x86_64` (`amd64`) and `i686` (32-bit) architectures, download the latest `.exe` file from the release links above. The installer will automatically register the host with the browser.

For `arm64`, there is no native Windows build available. However, you can use the `i686` version on Windows 10 or the `amd64` version on Windows 11, which will run in emulation mode.

> [!NOTE]
> It is possible to build the application for `arm64` natively, but, at the time of writing, there is no official support for this architecture in the Windows build scripts.
> Cross-compilation for `arm64` is not yet implemented because the MinGW toolchain does not support it. Building for `arm64` natively on Windows is possible but requires a Windows ARM64 machine
> with build tools installed, which is not available in the maintainer's CI/CD environment.

---

## Uninstallation

### RPM

```bash
sudo rpm -e beectl
```

### DEB

```bash
sudo apt purge beectl
```

---

## Building from Source

The project uses CMake and platform-specific toolchains located in `CMake/Toolchain-*.cmake`. The default compiler is GCC for Linux and MinGW-GCC for Windows.

### Cross-Compilation

To cross-compile for different platforms, you can use the `build-cross.sh` script. This script requires Docker to be installed and configured:

```bash
./build-cross.sh
```

### Native Linux (amd64)

```bash
./build-linux-amd64.sh -b Release
```

### Native Linux (i386)

```bash
./build-linux-i386.sh -b Release
```

### Cross-Compilation for Windows (i686)

```bash
./build-win-i686.sh -b Release
```

### Other Architectures

Custom toolchains can be used as follows:

```bash
./build.sh /path/to/custom-toolchain.cmake -b Release
```

Toolchains for other CPU architectures can be added on request.

### Debug Builds

By default, `./build.sh` builds a debug version if `-b` is not specified. Example:

```bash
./build.sh all -b Debug
```

This will iterate over all `Toolchain-*.cmake` files in the `CMake` directory.

To cross-compile for all platforms, you can use:

```bash
./build-cross.sh -b Debug
```

---

## Packaging

The build scripts (`build.sh`, `build-cross.sh` and others) automatically generate CPack configuration files.

To manually create a package, you can run:

```bash
make package
```

CPack will generate a package appropriate to your current platform (e.g., RPM, DEB, NSIS).

## License

This project is licensed under the [MIT License](LICENSE).

Copyright (C) 2019–2025 Ruslan Osmanov.
