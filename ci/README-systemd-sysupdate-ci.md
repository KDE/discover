# SystemD-Sysupdate Backend CI Testing

This document describes the CI pipeline setup for testing the SystemD-Sysupdate backend in Discover.

## Overview

The SystemD-Sysupdate backend requires the latest git master version of systemd to test properly. This presents some challenges for CI testing:

1. Most CI images don't include the latest systemd
2. Building systemd from source requires significant dependencies
3. Testing may require systemd-nspawn and mkosi for containerized testing

## CI Jobs

### 1. `test_systemd_tooling`

This job tests whether the necessary tools (mkosi, systemd-nspawn) are available and functional in the KDE CI environment.

**Purpose:** Determine if we can use systemd-nspawn and mkosi for containerized testing
**Status:** Manual trigger only
**Key checks:**
- systemd version and availability
- systemd-nspawn presence and execution
- mkosi availability
- Kernel capabilities (namespaces, cgroups)
- Container detection

**Run this job first** to determine the capabilities of the CI environment.

### 2. `test_systemd_sysupdate_backend`

This job builds systemd from git master and tests the Discover systemd-sysupdate backend against it.

**Purpose:** Full integration testing with latest systemd
**Status:** Manual trigger only
**Process:**
1. Install build dependencies
2. Clone systemd from git
3. Build systemd with sysupdate support enabled
4. Install to temporary location
5. Start mock systemd-sysupdate D-Bus service
6. Build Discover with SystemdSysupdateBackend enabled
7. Run tests

## Testing Approaches

### Approach 1: Mock D-Bus Service (Currently Implemented)

The backend includes a Python mock server (`libdiscover/backends/SystemdSysupdateBackend/test/mockserver.py`) that simulates the systemd-sysupdate D-Bus interface.

**Advantages:**
- No root privileges required
- Fast execution
- Works in any CI environment
- Easy to test edge cases

**Limitations:**
- Doesn't test against real systemd implementation
- May miss integration issues

**Environment Variable:**
Set `DISCOVER_TEST_SYSUPDATE=1` to enable the mock server in test mode.

### Approach 2: Build systemd from Source

Build the latest systemd from git master in CI.

**Advantages:**
- Tests against real systemd code
- Catches integration issues early
- No custom image needed

**Limitations:**
- Longer build times
- Requires many build dependencies
- May still need privileges for full testing

### Approach 3: Custom CI Image (Future)

Create a custom Docker/VM image with:
- Latest systemd built from git
- mkosi pre-installed
- systemd-nspawn configured
- All necessary privileges

**Advantages:**
- Fastest test execution
- Most realistic testing environment
- Reusable across CI runs

**Limitations:**
- Requires image maintenance
- Needs KDE sysadmin approval and setup

## Building a Custom Image

If the test jobs show that we need a custom image, use the following approach:

### Option A: Dockerfile

See `ci/Dockerfile.systemd-sysupdate` for a reference implementation.

### Option B: mkosi Configuration

See `ci/mkosi.default` for building a VM image with mkosi.

## Running Tests Locally

```bash
# Start the mock server
export DISCOVER_TEST_SYSUPDATE=1
python3 libdiscover/backends/SystemdSysupdateBackend/test/mockserver.py &

# Build discover
cmake -B build -DBUILD_SystemdSysupdateBackend=ON
cmake --build build

# Run tests
cd build
ctest -R systemd --output-on-failure

# Stop mock server
killall mockserver.py
```

## Next Steps

1. **Run `test_systemd_tooling` job** in GitLab CI to assess current capabilities
2. **Review the output** to determine which approach is viable
3. **If needed, create custom image** following the guidelines above
4. **Work with KDE sysadmin** to integrate custom image into ci-utilities

## References

- [SystemD GitHub](https://github.com/systemd/systemd)
- [mkosi Documentation](https://github.com/systemd/mkosi)
- [KDE CI Documentation](https://community.kde.org/Infrastructure/Continuous_Integration_System)
- [systemd-nspawn man page](https://www.freedesktop.org/software/systemd/man/systemd-nspawn.html)
