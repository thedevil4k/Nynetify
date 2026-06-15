#!/bin/bash
set -e

echo "=== Building Nynetify Debian Package ==="

# Ensure directories exist
mkdir -p build

# Build the project
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/nynetify
cmake --build build

# Prepare packaging directory
echo "=== Preparing packaging directory ==="
mkdir -p dist/opt/nynetify/bin
mkdir -p dist/opt/nynetify/lib
mkdir -p dist/usr/bin

# Copy the main binary
cp build/Nynetify dist/opt/nynetify/bin/

# Bundle mpv and its non-system library dependencies
echo "=== Bundling mpv dependencies ==="
cp /usr/bin/mpv dist/opt/nynetify/bin/
ldd /usr/bin/mpv | grep '=> /' | awk '{print $3}' | while read -r lib; do
    base="$(basename "$lib")"
    case "$base" in
        linux-vdso*|ld-linux*|libc.so*|libm.so*|libpthread*|libdl.so*|librt.so*|libstdc++*|libgcc_s*|libz.so*|libzstd*)
            ;;
        *)
            if [ -f "$lib" ]; then
                cp -L "$lib" dist/opt/nynetify/lib/ 2>/dev/null || true
            fi
            ;;
    esac
done

# Also copy Nynetify's non-system lib deps
ldd build/Nynetify | grep '=> /' | awk '{print $3}' | while read -r lib; do
    base="$(basename "$lib")"
    case "$base" in
        linux-vdso*|ld-linux*|libc.so*|libm.so*|libpthread*|libdl.so*|librt.so*|libstdc++*)
            ;;
        *)
            if [ -f "$lib" ] && [ ! -f "dist/opt/nynetify/lib/$base" ]; then
                cp -L "$lib" dist/opt/nynetify/lib/ 2>/dev/null || true
            fi
            ;;
    esac
done

# Bundle yt-dlp
echo "=== Bundling yt-dlp ==="
wget -q "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp" \
    -O dist/opt/nynetify/bin/yt-dlp || true
chmod +x dist/opt/nynetify/bin/yt-dlp 2>/dev/null || true

# Create launcher wrapper
echo "=== Creating launcher wrapper ==="
cp packaging/linux/nynetify.wrapper dist/opt/nynetify/nynetify
chmod +x dist/opt/nynetify/nynetify

# Create symlink
ln -s /opt/nynetify/nynetify dist/usr/bin/nynetify

# Build package using dpkg-deb
echo "=== Building .deb package ==="
mkdir -p dist/DEBIAN
cp packaging/linux/control dist/DEBIAN/control
dpkg-deb -b dist /app/nynetify-1.0.0-amd64.deb

echo "=== Build complete: /app/nynetify-1.0.0-amd64.deb ==="
