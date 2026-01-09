# Quick Start Guide - SystemD-Sysupdate Backend CI Testing

## What Was Added

### 1. CI Pipeline Jobs (.gitlab-ci.yml)

Two new manual CI jobs have been added:

- **`test_systemd_tooling`** - Tests if mkosi and systemd-nspawn work in KDE CI
- **`test_systemd_sysupdate_backend`** - Full build and test with latest systemd

### 2. Supporting Files (ci/ directory)

- **README-systemd-sysupdate-ci.md** - Comprehensive documentation
- **Dockerfile.systemd-sysupdate** - Custom Docker image with systemd from git
- **build-systemd-image.sh** - Script to build the Docker image
- **mkosi.default** - mkosi configuration for VM image
- **test-local-environment.sh** - Test your local environment
- **systemd-sysupdate-template.yml** - Template for ci-utilities contribution

## Quick Start

### Step 1: Test Locally (Optional)

```bash
cd discover
./ci/test-local-environment.sh
```

### Step 2: Test in GitLab CI

1. **Commit and push your changes**
   ```bash
   git add .gitlab-ci.yml ci/
   git commit -m "Add CI pipeline for systemd-sysupdate backend testing"
   git push
   ```

2. **Go to GitLab CI/CD → Pipelines**

3. **Manually trigger `test_systemd_tooling`**
   - This will show what's available in the CI environment
   - Takes ~2-5 minutes

4. **Review the output**
   - If mkosi and systemd-nspawn work → Great! Use Approach 2
   - If they don't work → Need to build custom image (Approach 3)

### Step 3: Full Test (After Step 2)

**Manually trigger `test_systemd_sysupdate_backend`**
- This builds systemd from git master
- Tests the SystemdSysupdateBackend
- Takes ~30-40 minutes

## Three Approaches

### Approach 1: Mock Server (Current - Works Now)
✅ No changes needed, already working
- Uses Python mock in `libdiscover/backends/SystemdSysupdateBackend/test/mockserver.py`
- Set `DISCOVER_TEST_SYSUPDATE=1` to use it

### Approach 2: Build systemd in CI (Implemented)
⏱️ Build time: ~30-40 minutes per run
- Uses `test_systemd_sysupdate_backend` job
- No custom image needed
- Good for occasional testing

### Approach 3: Custom Image (Best for frequent testing)
⚡ Test time: ~5-10 minutes per run
- Build once: `./ci/build-systemd-image.sh --build-only`
- Push to KDE registry (requires permissions)
- Update job to use custom image

## Building Custom Image

```bash
# Build locally
cd discover/ci
./build-systemd-image.sh --build-only

# Test the image
./build-systemd-image.sh --test

# Push to registry (requires KDE permissions)
docker login invent-registry.kde.org
./build-systemd-image.sh --push --tag v1.0
```

## Decision Tree

```
Do you need to test systemd-sysupdate backend?
│
├─ Rarely (few times a year)
│  └─ Use Approach 2: Build in CI
│     └─ Trigger test_systemd_sysupdate_backend manually
│
├─ Occasionally (monthly)
│  └─ Use Approach 2: Build in CI
│     └─ Consider Approach 3 if builds are slow
│
└─ Frequently (weekly/daily)
   └─ Use Approach 3: Custom Image
      └─ Work with KDE sysadmin to:
         1. Build and push image
         2. Add to ci-utilities
         3. Use in regular pipeline
```

## Next Steps

1. **Immediate:** Run `test_systemd_tooling` to assess CI capabilities
2. **Short-term:** Use `test_systemd_sysupdate_backend` for testing
3. **Long-term:** If needed frequently, build custom image and work with KDE sysadmin

## Questions?

See [ci/README-systemd-sysupdate-ci.md](README-systemd-sysupdate-ci.md) for detailed documentation.
