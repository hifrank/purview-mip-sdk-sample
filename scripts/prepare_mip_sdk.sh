#!/bin/bash
# Prepare the mip_sdk directory with headers + Linux .so files from the Ubuntu NuGet package.
# Run this before building the Docker image.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
MIP_SDK_DIR="$PROJECT_DIR/mip_sdk"

# NuGet package paths
NUGET_BASE="$HOME/.nuget/packages/microsoft.informationprotection.file.ubuntu2204/1.18.124/build/native"
NUGET_WIN="$HOME/.nuget/packages/microsoft.informationprotection.file/1.18.124/build/native"

# Use Ubuntu package headers (preferred), fallback to Windows package headers
if [ -d "$NUGET_BASE/include" ]; then
    HEADER_SRC="$NUGET_BASE/include"
elif [ -d "$NUGET_WIN/include" ]; then
    HEADER_SRC="$NUGET_WIN/include"
else
    echo "ERROR: MIP SDK NuGet packages not found. Run:"
    echo "  dotnet restore labeler-dotnet/MipLabeler/MipLabeler.csproj"
    exit 1
fi

BINS_SRC="$NUGET_BASE/bins/release/x86_64"

if [ ! -d "$BINS_SRC" ]; then
    echo "ERROR: Ubuntu MIP SDK binaries not found at $BINS_SRC"
    echo "Make sure Microsoft.InformationProtection.File.Ubuntu2204 NuGet package is restored."
    exit 1
fi

echo "=== Preparing MIP SDK directory ==="
echo "  Headers: $HEADER_SRC"
echo "  Bins:    $BINS_SRC"

# Copy headers
rm -rf "$MIP_SDK_DIR/include"
cp -R "$HEADER_SRC" "$MIP_SDK_DIR/include"
echo "  Copied headers to $MIP_SDK_DIR/include"

# Copy Linux .so files
mkdir -p "$MIP_SDK_DIR/bins/release"
cp "$BINS_SRC"/*.so* "$MIP_SDK_DIR/bins/release/" 2>/dev/null || true
echo "  Copied .so files to $MIP_SDK_DIR/bins/release/"

echo ""
echo "  Files copied:"
ls -la "$MIP_SDK_DIR/bins/release/"
echo ""
echo "=== Done. Ready to build Docker image ==="
echo "  cd $PROJECT_DIR && docker build -t mip-labeler -f labeler/Dockerfile ."
