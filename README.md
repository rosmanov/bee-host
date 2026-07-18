# Bee Native Messaging Host

A native messaging host application for the [Browser's External Editor extension](https://github.com/rosmanov/chrome-bee).

## Supported Operating Systems

- Linux
- Windows
- macOS
- FreeBSD

---

## Installation

### Prebuilt Packages

Packages are available from:

- [SourceForge](https://sourceforge.net/projects/beectl/)
- [GitHub Releases page](https://github.com/rosmanov/bee-host/releases)
- [FreeBSD Port](https://www.freshports.org/editors/bee-host/)
- [Fedora Copr](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/)

### macOS

Download the latest `.pkg` package and install it:

```bash
sudo spctl --master-disable
sudo installer -pkg ./beectl-1.3.7-3.x86_64.pkg -target /
sudo spctl --master-enable
```

> [!NOTE]
> Gatekeeper must be temporarily disabled because the package is not signed with an Apple Developer certificate.

### Fedora / CentOS / openSUSE (RPM)

[![Copr build status](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/package/beectl/status_image/last_build.png?a)](https://copr.fedorainfracloud.org/coprs/ruslan-osmanov/beectl/package/beectl/)

```bash
sudo dnf copr enable ruslan-osmanov/beectl
sudo dnf install --refresh beectl
```

Or install manually:

```bash
rpm -Uvh --nodeps beectl-<VERSION>.<ARCH>.rpm
```

### Debian / Ubuntu (DEB)

```bash
dpkg -i beectl-<VERSION>.<ARCH>.deb
```

### Arch Linux

Install from the AUR using **yay**:
```bash
yay -S beectl
```

Alternatively,
```bash
git clone https://aur.archlinux.org/beectl.git
cd beectl
makepkg -si
```

The package installs the binary and browser native messaging manifests automatically.

### NixOS

Add BeeCtl to your configuration:

```
environment.systemPackages = [ pkgs.beectl ];

programs.firefox.nativeMessagingHosts.packages = [ pkgs.beectl ];
```

#### Nix Package Manager (non-NixOS)

Install:

```bash
nix profile install github:rosmanov/bee-host --extra-experimental-features "nix-command flakes"
```

Create browser manifest links:

```bash
mkdir -p ~/.mozilla/native-messaging-hosts
ln -sf ~/.nix-profile/lib/mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json \
       ~/.mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json
```

For Chrome:

```
mkdir -p ~/.config/google-chrome/NativeMessagingHosts
ln -sf ~/.nix-profile/etc/opt/chrome/native-messaging-hosts/com.ruslan_osmanov.bee.json \
       ~/.config/google-chrome/NativeMessagingHosts/com.ruslan_osmanov.bee.json
```

For Chromium:

```
mkdir -p ~/.config/chromium/NativeMessagingHosts
ln -sf ~/.nix-profile/etc/chromium/native-messaging-hosts/com.ruslan_osmanov.bee.json \
       ~/.config/chromium/NativeMessagingHosts/com.ruslan_osmanov.bee.json
```

Upgrade:

```
nix profile upgrade '.*bee.*' --refresh --extra-experimental-features "nix-command flakes"
```

The links do not need to be recreated after upgrades.

### Windows

Download and run the latest `.exe` installer. It registers the native messaging host automatically.

During installation:

- enable adding `beectl` to PATH (recommended),
- keep both `application` and `config` components selected.

If SmartScreen blocks the installer, choose **More info** → **Run anyway**.

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

### Arch Linux

```bash
sudo pacman -R beectl
```

---

## Building from Source

The project uses CMake with platform-specific toolchains in `CMake/Toolchain-*.cmake`.

### Automated Builds

GitHub Actions builds packages for supported platforms and architectures.

Builds run automatically on pushes and pull requests. They can also be triggered manually with workflow_dispatch.

Run locally with [act](https://github.com/nektos/act):

```bash
act -b -j build-linux
```

### Native Builds

Linux (amd64):

```bash
./build-linux-i386.sh -b Release
```

macOS:

```bash
./build-macos.sh -b Release
```

Windows (PowerShell):

First, install [NSIS](https://nsis.sourceforge.io/) (for packaging) and ensure it is in your `PATH` (e.g., via Chocolatey: `choco install nsis -y`).

Then configure, build, and package:

```powershell
cmake -B build -S . -DCPACK_GENERATOR="NSIS;ZIP" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
cmake --build build --config Release
cd build
cpack -C Release
```

Custom toolchain:

```bash
./build.sh /path/to/toolchain.cmake -b Release
```

Debug build:

```bash
./build.sh all -b Debug
```

## Packaging

Build scripts generate CPack configuration automatically.

Create a package manually:

```bash
make package
```

CPack generates a platform-specific package (RPM, DEB, NSIS, etc.).

## License

This project is licensed under the [MIT License](LICENSE).

Copyright (C) 2019–2026 Ruslan Osmanov.
