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

### Arch Linux

BeeCtl is available as an Arch Linux package built natively from source using the provided `PKGBUILD`.

Ensure you have the standard Arch Linux package building prerequisites installed:

```bash
sudo pacman -S --needed base-devel
```

To build and install the package:

1. Clone this repository:
   ```bash
   git clone https://github.com/rosmanov/bee-host.git
   cd bee-host
   ```
2. Navigate to the `arch` directory and build using `makepkg`:
   ```bash
   cd arch
   makepkg -si
   ```

This will automatically compile the binary using system libraries, install it to `/usr/bin/beectl`, and place the browser manifests in the correct standard system directories for Chrome, Chromium, and Firefox:
- Firefox manifest: `/usr/lib/mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json`
- Chrome manifest: `/etc/opt/chrome/native-messaging-hosts/com.ruslan_osmanov.bee.json`
- Chromium manifest: `/etc/chromium/native-messaging-hosts/com.ruslan_osmanov.bee.json`

### NixOS / Nix Package Manager

BeeCtl is available as a Nix flake for reproducible builds on NixOS and systems with Nix package manager.

#### NixOS Installation (Idiomatic)

On NixOS, you don't need to manually link manifests. Add the package to your configuration and configure your browser.

In your `flake.nix` or `configuration.nix`:

```nix
{ pkgs, ... }: {
  # 1. Add the package (assuming you added the flake to your inputs)
  environment.systemPackages = [ pkgs.beectl ];
  
  # 2. Configure Firefox to find the native messaging host
  programs.firefox.nativeMessagingHosts.packages = [ pkgs.beectl ];
  
  # Note: Chromium-based browsers may also require specific configuration
  # depending on your setup.
}
```

#### Nix Package Manager (Non-NixOS)

If you are using the Nix package manager on another Linux distribution, install the package using:

```bash
nix profile install github:rosmanov/bee-host --extra-experimental-features "nix-command flakes"
```

After installation, create symbolic links for browser manifests. **This is required because:**
- Browsers expect manifests in specific standard locations (`~/.mozilla/native-messaging-hosts/`, etc.)
- Nix installs packages to isolated read-only paths in `/nix/store/`
- The symlinks bridge these two locations

**For Firefox:**
```bash
mkdir -p ~/.mozilla/native-messaging-hosts
ln -sf ~/.nix-profile/lib/mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json \
       ~/.mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json
```

**For Chrome:**
```bash
mkdir -p ~/.config/google-chrome/NativeMessagingHosts
ln -sf ~/.nix-profile/etc/opt/chrome/native-messaging-hosts/com.ruslan_osmanov.bee.json \
       ~/.config/google-chrome/NativeMessagingHosts/com.ruslan_osmanov.bee.json
```

**For Chromium:**
```bash
mkdir -p ~/.config/chromium/NativeMessagingHosts
ln -sf ~/.nix-profile/etc/chromium/native-messaging-hosts/com.ruslan_osmanov.bee.json \
       ~/.config/chromium/NativeMessagingHosts/com.ruslan_osmanov.bee.json
```

> [!NOTE]
> You only need to create these symlinks once. They point to `~/.nix-profile/` which Nix automatically updates when you upgrade, so the manifests will always reference the current version.

#### Upgrading

```bash
nix profile upgrade '.*bee.*' --refresh --extra-experimental-features "nix-command flakes"
```

The manifests will automatically point to the new version - no need to recreate the symlinks.

#### Alternative: Use Precompiled TGZ

If you prefer to avoid manual manifest setup, download the TGZ package and extract to your home directory:

```bash
tar xzf beectl-<VERSION>-<RELEASE>.<ARCH>.Release.TGZ -C ~/.local --strip-components=1
```

Then create symlinks as shown above, replacing `~/.nix-profile/` with `~/.local/`.

### Windows

#### Step 1: Download the Installer

For the `x86_64` (`amd64`) and `i686` (32-bit) architectures, download the latest `.exe` file from the release links above. The installer will automatically register the host with the browser.

For `arm64`, there is no native Windows build available. However, you can use the `i686` version on Windows 10 or the `amd64` version on Windows 11, which will run in emulation mode.

> [!NOTE]
> It is possible to build the application for `arm64` natively, but, at the time of writing, there is no official support for this architecture in the Windows build scripts.
> Cross-compilation for `arm64` is not yet implemented because the MinGW toolchain does not support it. Building for `arm64` natively on Windows is possible but requires a Windows ARM64 machine
> with build tools installed, which is not available in the maintainer's CI/CD environment.

#### Step 2: Install the Host App

Run the downloaded installer. It will automatically register the host with the browser.

In the "Install Options" dialog, choose either of the following options:

- "Add beectl to the system PATH for all users" (recommended)"
- "Add beectl to the system PATH for the current user"

Click "Next" and accept the default destination folder.

Next, accept the default start menu folder "BeeCtl" unless you want to change it. Click "Next".

Please leave both components checked:
- "application"
- "config"

Click "Next" and then "Install".

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

### Cross-Platform Build (GitHub Actions)

The project includes a GitHub Actions workflow that automatically builds and packages the application for all 9 target combinations (Linux amd64/i386/arm/aarch64/ppc64le, Windows amd64/i686, macOS x86_64/arm64) using native runners and optimized cross-compilers.

To trigger the build, simply push to `master`/`main` or open a pull request. The workflow can also be triggered manually using the `workflow_dispatch` event on GitHub.

#### Running Cross-Platform Builds Locally (`act`)

You can easily run the GitHub Actions build pipeline locally using the [`act`](https://github.com/nektos/act) command line tool:

```bash
# Run the Linux builds locally
act -b -j build-linux
```

### Native Builds

For local native builds, you can use the target-specific scripts or run `cmake` directly.

#### Native Linux (amd64)

```bash
./build-linux-amd64.sh -b Release
```

#### Native Linux (i386)

```bash
./build-linux-i386.sh -b Release
```

#### Native macOS

```bash
./build-macos.sh -b Release
```

#### Other Architectures / Custom Toolchains

You can build with a custom toolchain file:

```bash
./build.sh /path/to/custom-toolchain.cmake -b Release
```

### Debug Builds

By default, `./build.sh` builds a debug version if `-b` is not specified. Example:

```bash
./build.sh all -b Debug
```

This will iterate over all `Toolchain-*.cmake` files in the `CMake` directory on macOS/Linux.

---

## Packaging

The build scripts (`build.sh` and others) automatically generate CPack configuration files.

To manually create a package, you can run:

```bash
make package
```

CPack will generate a package appropriate to your current platform (e.g., RPM, DEB, NSIS).

## License

This project is licensed under the [MIT License](LICENSE).

Copyright (C) 2019–2025 Ruslan Osmanov.
