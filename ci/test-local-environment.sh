#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2026 KDE Community

# Quick test script to check if mkosi and systemd-nspawn work locally

set -e

echo "=== Testing systemd and container tools locally ==="
echo ""

# Check systemd
echo "1. Checking systemd version..."
if command -v systemctl &> /dev/null; then
    systemctl --version | head -n 1
else
    echo "   ❌ systemd not found"
fi
echo ""

# Check systemd-nspawn
echo "2. Checking systemd-nspawn..."
if command -v systemd-nspawn &> /dev/null; then
    systemd-nspawn --version | head -n 1
    echo "   ✓ systemd-nspawn available"
else
    echo "   ❌ systemd-nspawn not found"
    echo "   Install with: sudo zypper install systemd-container"
fi
echo ""

# Check mkosi
echo "3. Checking mkosi..."
if command -v mkosi &> /dev/null; then
    mkosi --version
    echo "   ✓ mkosi available"
else
    echo "   ❌ mkosi not found"
    echo "   Install with: pip3 install --user mkosi"
fi
echo ""

# Check systemd-sysupdate
echo "4. Checking systemd-sysupdate..."
if command -v systemd-sysupdate &> /dev/null; then
    systemd-sysupdate --version | head -n 1
    echo "   ✓ systemd-sysupdate available"
else
    echo "   ⚠ systemd-sysupdate not found (this is expected, will be built from git)"
fi
echo ""

# Check user namespaces
echo "5. Checking user namespace support..."
if [ -f /proc/sys/kernel/unprivileged_userns_clone ]; then
    USERNS=$(cat /proc/sys/kernel/unprivileged_userns_clone)
    if [ "$USERNS" = "1" ]; then
        echo "   ✓ Unprivileged user namespaces enabled"
    else
        echo "   ⚠ Unprivileged user namespaces disabled"
        echo "   Enable with: sudo sysctl kernel.unprivileged_userns_clone=1"
    fi
else
    echo "   ⚠ Cannot determine user namespace support"
fi
echo ""

# Test namespace creation
echo "6. Testing namespace creation..."
if unshare --user --pid --fork echo "test" &> /dev/null; then
    echo "   ✓ Can create user and PID namespaces"
else
    echo "   ❌ Cannot create namespaces (may need privileges)"
fi
echo ""

# Check cgroups
echo "7. Checking cgroup support..."
if [ -d /sys/fs/cgroup ]; then
    echo "   ✓ cgroups available"
    CGROUP_VERSION=$(stat -fc %T /sys/fs/cgroup)
    echo "   Filesystem type: ${CGROUP_VERSION}"
else
    echo "   ❌ cgroups not available"
fi
echo ""

echo "=== Summary ==="
echo "If all checks pass, you can run the CI test jobs to verify they work in GitLab CI."
echo "Otherwise, you may need to build a custom CI image with the necessary tools."
echo ""
echo "Next steps:"
echo "1. Push changes and trigger 'test_systemd_tooling' job in GitLab CI"
echo "2. Review the job output"
echo "3. If needed, build custom image with: ci/build-systemd-image.sh --build-only"
