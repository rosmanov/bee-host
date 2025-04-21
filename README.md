# About

A native messaging host application for [Browser's Exernal Editor extension](https://github.com/rosmanov/chrome-bee).

## Supported Operating Systems

- GNU/Linux
- Windows (MinGW binaries*)
- macOS (tested on 10.15.6)
- FreeBSD

# Installing

- Precompiled binaries can be downloaded from [SourceForge](https://sourceforge.net/projects/beectl/) or from the [releases page](https://github.com/rosmanov/bee-host/releases).
- [**FreeBSD** port](https://www.freshports.org/editors/bee-host/).

## macOS

Download the latest `.pkg` file from the release links provided above. Then run the following commands (replace `./beectl-1.3.7-3.x86_64.Release.pkg` with the path to the downloaded package):
```
sudo spctl --master-disable
sudo installer -pkg ./beectl-1.3.7-3.x86_64.Release.pkg -target /
sudo spctl --master-enable
```
> [!NOTE]
> The `spctl` commands temporarily disable macOS Gatekeeper to allow the installation of unsigned packages. I currently don’t have an individual Apple Developer license, so I can’t sign the package with an officially accepted certificate. I’m not willing to spend $99 per year just for this purpose—sorry.

## RPM

[![Copr build status](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/package/beectl/status_image/last_build.png?a)](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/package/beectl/)

[This RPM repository](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/) can be used to install beectl using a package manager such as `dnf`, e.g.:

```bash
sudo dnf copr enable ruslan-osmanov/beectl
sudo dnf install --refresh beectl
```

Alternatively, download the file from SourceForge or GitHub, then install it:

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
