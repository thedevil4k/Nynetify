#!/bin/bash
set -e

echo "=== Building Nynetify RPM Package ==="

VERSION="1.0.0"
ARCH="x86_64"

# Ensure directories exist
mkdir -p build

# Build the project
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/nynetify
cmake --build build

# Prepare RPM build environment
RPM_BUILD_DIR="/root/rpmbuild"
mkdir -p "$RPM_BUILD_DIR/BUILD"
mkdir -p "$RPM_BUILD_DIR/RPMS/$ARCH"
mkdir -p "$RPM_BUILD_DIR/BUILDROOT"
mkdir -p "$RPM_BUILD_DIR/SPECS"
mkdir -p "$RPM_BUILD_DIR/SOURCES"

BR="$RPM_BUILD_DIR/BUILDROOT/nynetify-${VERSION}-1.${ARCH}"
mkdir -p "$BR/opt/nynetify/bin"
mkdir -p "$BR/opt/nynetify/lib"
mkdir -p "$BR/usr/bin"

# 1. Nynetify binary
cp build/Nynetify "$BR/opt/nynetify/bin/"

# 2. Bundle mpv + its non-system library dependencies
cp /usr/bin/mpv "$BR/opt/nynetify/bin/"
ldd /usr/bin/mpv | grep '=> /' | awk '{print $3}' | while read -r lib; do
    base="$(basename "$lib")"
    case "$base" in
        linux-vdso*|ld-linux*|libc.so*|libm.so*|libpthread*|libdl.so*|librt.so*|libstdc++*|libgcc_s*|libz.so*|libzstd*)
            ;;
        *)
            if [ -f "$lib" ]; then
                cp -L "$lib" "$BR/opt/nynetify/lib/" 2>/dev/null || true
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
            if [ -f "$lib" ] && [ ! -f "$BR/opt/nynetify/lib/$base" ]; then
                cp -L "$lib" "$BR/opt/nynetify/lib/" 2>/dev/null || true
            fi
            ;;
    esac
done

# 3. Bundle yt-dlp
wget -q "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp" \
    -O "$BR/opt/nynetify/bin/yt-dlp" || true
chmod +x "$BR/opt/nynetify/bin/yt-dlp" 2>/dev/null || true

# 4. Launcher wrapper
cp packaging/linux/nynetify.wrapper "$BR/opt/nynetify/nynetify"
chmod +x "$BR/opt/nynetify/nynetify"

# 5. Symlink
ln -s /opt/nynetify/nynetify "$BR/usr/bin/nynetify"

# 6. SPEC file
cp packaging/linux/nynetify.spec /root/rpmbuild/SPECS/nynetify.spec

# 7. Build RPM
rpmbuild -bb /root/rpmbuild/SPECS/nynetify.spec --buildroot "$BR" --target "$ARCH"

# Copy the resulting RPM to /app/
cp "$RPM_BUILD_DIR/RPMS/$ARCH/nynetify-1.0.0-1.${ARCH}.rpm" /app/

echo "=== Build complete: /app/nynetify-1.0.0-1.${ARCH}.rpm ==="
