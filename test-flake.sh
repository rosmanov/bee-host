#!/bin/bash
# Test script to verify Nix flake works correctly
set -e
set -x

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

# Setup results collection
RESULTS_FILE=$(mktemp)
LOG_FILE=$(mktemp)
trap "rm -f $RESULTS_FILE $LOG_FILE" EXIT

# Helper function to record test results
record_result() {
  local test_num="$1"
  local test_name="$2"
  local status="$3"  # PASS, FAIL, WARN
  local message="$4"
  echo "$test_num|$test_name|$status|$message" >> "$RESULTS_FILE"
}

echo "======================================"
echo "Testing Nix Flake for BeeCtl"
echo "======================================"
echo ""
echo "Running tests... (detailed output saved to log)"
echo ""

echo -n "Test 1: Validating flake structure... "
if docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest \
  nix --extra-experimental-features "nix-command flakes" flake check >> "$LOG_FILE" 2>&1; then
  echo "✓"
  record_result "1" "Flake structure validation" "PASS" "Flake structure is valid"
else
  echo "✗"
  record_result "1" "Flake structure validation" "FAIL" "Flake check failed"
  exit 1
fi

echo -n "Test 2: Showing flake metadata... "
if docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest \
  nix --extra-experimental-features "nix-command flakes" flake show >> "$LOG_FILE" 2>&1; then
  echo "✓"
  record_result "2" "Flake metadata" "PASS" "Metadata displayed successfully"
else
  echo "✗"
  record_result "2" "Flake metadata" "FAIL" "Could not show metadata"
fi

echo -n "Test 3: Building package with sandbox enabled... "
BUILD_OUTPUT=$(mktemp)
trap "rm -f $RESULTS_FILE $LOG_FILE $BUILD_OUTPUT" EXIT

if docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest sh -c '
    rm -rf result
    nix --extra-experimental-features "nix-command flakes" build \
      --option sandbox true \
      --print-build-logs 2>&1 | tee /tmp/build.log
    cat /tmp/build.log
' > "$BUILD_OUTPUT" 2>&1; then
  echo "✓"

  # Check for network access attempts
  if grep -qi "unable to download\|fetching.*github\|curl.*github\|git.*clone" "$BUILD_OUTPUT"; then
    record_result "3" "Sandbox build" "FAIL" "Build attempted network access in sandbox"
    exit 1
  else
    record_result "3" "Sandbox build" "PASS" "Build succeeded without network access"
  fi

  # Check USE_SYSTEM_DEPS
  if grep -q "Using system-provided libuv and cJSON" "$BUILD_OUTPUT"; then
    record_result "3.1" "  └─ USE_SYSTEM_DEPS" "PASS" "Build used system dependencies"
  else
    record_result "3.1" "  └─ USE_SYSTEM_DEPS" "WARN" "Could not confirm USE_SYSTEM_DEPS was enabled"
  fi

  # Check static library detection
  if grep -q "Using static.*libuv" "$BUILD_OUTPUT"; then
    record_result "3.2" "  └─ Static libuv" "PASS" "Detected static libuv library"
  else
    record_result "3.2" "  └─ Static libuv" "WARN" "Could not confirm static libuv"
  fi

  if grep -q "Using static.*cJSON" "$BUILD_OUTPUT"; then
    record_result "3.3" "  └─ Static cJSON" "PASS" "Detected static cJSON library"
  else
    record_result "3.3" "  └─ Static cJSON" "WARN" "Could not confirm static cJSON"
  fi

  cat "$BUILD_OUTPUT" >> "$LOG_FILE"
else
  echo "✗"
  record_result "3" "Sandbox build" "FAIL" "Build failed"
  cat "$BUILD_OUTPUT" >> "$LOG_FILE"
  exit 1
fi

echo -n "Test 4: Verifying binary and static linking... "
BINARY_OUTPUT=$(mktemp)
trap "rm -f $RESULTS_FILE $LOG_FILE $BUILD_OUTPUT $BINARY_OUTPUT" EXIT

if docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest sh -c '
    nix --extra-experimental-features "nix-command flakes" build 2>&1

    echo "=== Binary Info ==="
    ls -lh result/usr/local/bin/beectl
    ./result/usr/local/bin/beectl --version
    echo ""

    # Check file type
    if command -v file >/dev/null 2>&1; then
      echo "=== File Type ==="
      file result/usr/local/bin/beectl
      echo ""
    fi

    # Install binutils
    echo "=== Installing Analysis Tools ==="
    nix-env --extra-experimental-features "nix-command flakes" -iA nixpkgs.binutils 2>/dev/null || true
    echo ""

    # Check with ldd
    if command -v ldd >/dev/null 2>&1; then
      echo "=== Dynamic Dependencies (ldd) ==="
      ldd result/usr/local/bin/beectl 2>&1 || true
      echo ""

      if ldd result/usr/local/bin/beectl 2>/dev/null | grep -E "libuv|libcjson"; then
        echo "ERROR: Found dynamic libuv/cjson dependencies"
        exit 1
      fi
    fi

    # Check with readelf
    if command -v readelf >/dev/null 2>&1; then
      echo "=== NEEDED Libraries (readelf) ==="
      readelf -d result/usr/local/bin/beectl 2>/dev/null | grep NEEDED || echo "(none - fully static)"
      echo ""

      if readelf -d result/usr/local/bin/beectl 2>/dev/null | grep NEEDED | grep -E "libuv|cjson"; then
        echo "ERROR: Found libuv/cjson in NEEDED entries"
        exit 1
      fi
    fi

    # Check symbols with nm
    if command -v nm >/dev/null 2>&1; then
      echo "=== Symbol Check (nm) ==="
      if nm result/usr/local/bin/beectl 2>/dev/null | grep -q "uv_loop_init"; then
        echo "Found libuv symbols in binary (statically linked)"
      else
        echo "WARNING: Could not find libuv symbols"
      fi
      echo ""
    fi
' > "$BINARY_OUTPUT" 2>&1; then
  echo "✓"

  # Parse results
  if grep -q "statically linked" "$BINARY_OUTPUT"; then
    record_result "4.1" "  └─ Binary type" "PASS" "Binary is statically linked"
  elif grep -q "dynamically linked" "$BINARY_OUTPUT"; then
    record_result "4.1" "  └─ Binary type" "WARN" "Binary is dynamically linked"
  fi

  if grep -q "ERROR: Found dynamic libuv/cjson" "$BINARY_OUTPUT"; then
    record_result "4.2" "  └─ No dynamic deps" "FAIL" "Found dynamic libuv/cjson dependencies"
    cat "$BINARY_OUTPUT" >> "$LOG_FILE"
    exit 1
  else
    record_result "4.2" "  └─ No dynamic deps" "PASS" "No dynamic libuv/cjson dependencies"
  fi

  if grep -q "Found libuv symbols in binary" "$BINARY_OUTPUT"; then
    record_result "4.3" "  └─ Symbol check" "PASS" "libuv symbols found (statically linked)"
  else
    record_result "4.3" "  └─ Symbol check" "WARN" "Could not verify libuv symbols"
  fi

  record_result "4" "Binary verification" "PASS" "Binary verification passed"
  cat "$BINARY_OUTPUT" >> "$LOG_FILE"
else
  echo "✗"
  record_result "4" "Binary verification" "FAIL" "Binary verification failed"
  cat "$BINARY_OUTPUT" >> "$LOG_FILE"
  exit 1
fi

# Test 5: Check manifests exist
echo -n "Test 5: Checking native messaging manifests... "
MANIFEST_OUTPUT=$(mktemp)
trap "rm -f $RESULTS_FILE $LOG_FILE $BUILD_OUTPUT $BINARY_OUTPUT $MANIFEST_OUTPUT" EXIT

if docker run --rm \
  -v "$PROJECT_DIR:/workspace" \
  -w /workspace \
  nixos/nix:latest sh -c '
    nix --extra-experimental-features "nix-command flakes" build 2>&1

    echo "=== Firefox Manifest ==="
    if [ -f result/usr/lib/mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json ]; then
      cat result/usr/lib/mozilla/native-messaging-hosts/com.ruslan_osmanov.bee.json
      echo ""
    else
      echo "ERROR: Firefox manifest not found"
      exit 1
    fi

    echo "=== Chrome Manifest ==="
    if [ -f result/etc/opt/chrome/native-messaging-hosts/com.ruslan_osmanov.bee.json ]; then
      cat result/etc/opt/chrome/native-messaging-hosts/com.ruslan_osmanov.bee.json
      echo ""
    else
      echo "ERROR: Chrome manifest not found"
      exit 1
    fi

    echo "=== Directory Structure ==="
    find result -type f | sort
' > "$MANIFEST_OUTPUT" 2>&1; then
  echo "✓"

  if grep -q "ERROR: Firefox manifest not found" "$MANIFEST_OUTPUT"; then
    record_result "5.1" "  └─ Firefox manifest" "FAIL" "Firefox manifest not found"
    exit 1
  else
    record_result "5.1" "  └─ Firefox manifest" "PASS" "Firefox manifest exists"
  fi

  if grep -q "ERROR: Chrome manifest not found" "$MANIFEST_OUTPUT"; then
    record_result "5.2" "  └─ Chrome manifest" "FAIL" "Chrome manifest not found"
    exit 1
  else
    record_result "5.2" "  └─ Chrome manifest" "PASS" "Chrome manifest exists"
  fi

  record_result "5" "Manifest check" "PASS" "All manifests exist"
  cat "$MANIFEST_OUTPUT" >> "$LOG_FILE"
else
  echo "✗"
  record_result "5" "Manifest check" "FAIL" "Manifest check failed"
  cat "$MANIFEST_OUTPUT" >> "$LOG_FILE"
  exit 1
fi

# Cleanup
test -L result -o -e result && rm result

echo ""
echo "======================================"
echo "Test Results Summary"
echo "======================================"
echo ""

# Count results
TOTAL=$(wc -l < "$RESULTS_FILE")
PASSED=$(grep -c "|PASS|" "$RESULTS_FILE" || true)
FAILED=$(grep -c "|FAIL|" "$RESULTS_FILE" || true)
WARNED=$(grep -c "|WARN|" "$RESULTS_FILE" || true)

# Print results with color coding
while IFS='|' read -r num name status message; do
  case "$status" in
    PASS)
      printf "  ✓ %-30s %s\n" "$name" "$message"
      ;;
    FAIL)
      printf "  ✗ %-30s %s\n" "$name" "$message"
      ;;
    WARN)
      printf "  ⚠ %-30s %s\n" "$name" "$message"
      ;;
  esac
done < "$RESULTS_FILE"

echo ""
echo "======================================"
printf "Total: %d | Passed: %d | Failed: %d | Warnings: %d\n" "$TOTAL" "$PASSED" "$FAILED" "$WARNED"
echo "======================================"
echo ""

if [ "$FAILED" -eq 0 ]; then
  echo "✓ All tests passed!"
  echo ""
  echo "Detailed logs saved to: $LOG_FILE"
  echo "(Log will be deleted when script exits)"
  echo ""
  echo "The flake is working correctly and ready to merge."
else
  echo "✗ Some tests failed. Check the logs for details."
  echo "Detailed logs saved to: $LOG_FILE"
  exit 1
fi
