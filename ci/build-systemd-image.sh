#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2026 KDE Community

# Script to build and optionally push the systemd-sysupdate CI image

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKERFILE="${SCRIPT_DIR}/Dockerfile.systemd-sysupdate"
IMAGE_NAME="kde-discover-systemd-ci"
IMAGE_TAG="${IMAGE_TAG:-latest}"
REGISTRY="${REGISTRY:-invent-registry.kde.org}"
FULL_IMAGE_NAME="${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}"

usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Build a Docker image with latest systemd for CI testing of the SystemdSysupdateBackend.

OPTIONS:
    -h, --help          Show this help message
    -b, --build-only    Only build the image, don't push
    -p, --push          Build and push the image to registry
    -t, --tag TAG       Image tag (default: latest)
    -r, --registry REG  Registry URL (default: invent-registry.kde.org)
    --test              Build and run a test container

EXAMPLES:
    # Build only
    $0 --build-only

    # Build and test locally
    $0 --test

    # Build and push to registry (requires authentication)
    $0 --push --tag v1.0

NOTES:
    - Building can take 20-30 minutes as it compiles systemd from source
    - The resulting image will be approximately 1-2 GB
    - For pushing, you need to be authenticated with: docker login ${REGISTRY}

EOF
}

build_image() {
    echo "Building Docker image: ${FULL_IMAGE_NAME}"
    echo "This may take 20-30 minutes as systemd is built from source..."
    
    docker build \
        -t "${FULL_IMAGE_NAME}" \
        -f "${DOCKERFILE}" \
        "${SCRIPT_DIR}/.."
    
    echo "Build completed successfully!"
    echo "Image: ${FULL_IMAGE_NAME}"
}

push_image() {
    echo "Pushing image to registry: ${FULL_IMAGE_NAME}"
    docker push "${FULL_IMAGE_NAME}"
    echo "Push completed successfully!"
}

test_image() {
    echo "Testing image: ${FULL_IMAGE_NAME}"
    
    docker run --rm -it "${FULL_IMAGE_NAME}" /bin/bash -c "
        set -e
        echo '=== Testing systemd-sysupdate availability ==='
        systemd-sysupdate --version
        
        echo '=== Testing mkosi availability ==='
        mkosi --version
        
        echo '=== Testing systemd-nspawn availability ==='
        systemd-nspawn --version
        
        echo '=== All tests passed! ==='
    "
    
    echo "Test completed successfully!"
}

# Parse arguments
BUILD_ONLY=false
PUSH=false
TEST=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -p|--push)
            PUSH=true
            shift
            ;;
        -t|--tag)
            IMAGE_TAG="$2"
            FULL_IMAGE_NAME="${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}"
            shift 2
            ;;
        -r|--registry)
            REGISTRY="$2"
            FULL_IMAGE_NAME="${REGISTRY}/${IMAGE_NAME}:${IMAGE_TAG}"
            shift 2
            ;;
        --test)
            TEST=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Main execution
if [ "$BUILD_ONLY" = false ] && [ "$PUSH" = false ] && [ "$TEST" = false ]; then
    echo "No action specified. Use --help for usage information."
    echo "Common usage: $0 --build-only"
    exit 1
fi

build_image

if [ "$TEST" = true ]; then
    test_image
fi

if [ "$PUSH" = true ]; then
    push_image
fi

echo "Done!"
