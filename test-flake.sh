#!/bin/bash
# Test script to verify Nix flake works correctly
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

echo "======================================"
echo "Testing Nix Flake for BeeCtl"
echo "======================================"
echo ""

# Test 1: Flake structure validation
echo "Test 1: Validating flake structure..."
docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest \
  nix --extra-experimental-features "nix-command flakes" flake check
echo "✓ Flake structure is valid"
echo ""

# Test 2: Show flake metadata
echo "Test 2: Showing flake metadata..."
docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest \
  nix --extra-experimental-features "nix-command flakes" flake show
echo ""

# Test 3: Build the package
echo "Test 3: Building package..."
docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest \
  nix --extra-experimental-features "nix-command flakes" build --print-build-logs
echo "✓ Build successful"
echo ""

# Test 4: Verify binary and version
echo "Test 4: Verifying binary and version..."
docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest sh -c '
    nix --extra-experimental-features "nix-command flakes" build
    echo "Binary location:"
    ls -lh result/usr/local/bin/beectl
    echo ""
    echo "Testing binary:"
    ./result/usr/local/bin/beectl --version
    echo ""
    ./result/usr/local/bin/beectl --help | head -5
'
echo ""

# Test 5: Check manifests exist
echo "Test 5: Checking native messaging manifests..."
docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest sh -c '
    nix --extra-experimental-features "nix-command flakes" build
    echo "Firefox manifest:"
    cat result/usr/lib/mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json
    echo ""
    echo "Chrome manifest:"
    cat result/etc/opt/chrome/native-messaging-hosts/com.ruslan_osmanov.bee.json
    echo ""
    echo "Directory structure:"
    find result -type f | sort
'
echo ""

echo "======================================"
echo "✓ All tests passed!"
echo "======================================"
echo ""
echo "The flake is working correctly and ready to merge."
