#!/usr/bin/env bash
set -ex

# ==============================================================================
# Baudix Standalone Linux .deb Packaging Script
# Bundles Qt libraries, plugins, and dependencies for maximum compatibility
# across Debian 12+, Ubuntu 22.04+, and future Linux distributions.
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="${1:-$ROOT_DIR/build}"
OUTPUT_DIR="${2:-$BUILD_DIR}"

# Resolve BUILD_DIR to absolute path
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"
OUTPUT_DIR="$(mkdir -p "$OUTPUT_DIR" && cd "$OUTPUT_DIR" && pwd)"

if [ ! -f "$BUILD_DIR/baudix" ]; then
    echo "Error: Binary not found at $BUILD_DIR/baudix"
    echo "Please build the project first: cmake --build $BUILD_DIR --config Release"
    exit 1
fi

# Extract version from CMakeLists.txt or default to 1.3.5
VERSION=$(grep -oP 'project\(baudix VERSION \K[0-9.]+' "$ROOT_DIR/CMakeLists.txt" || echo "1.3.5")
echo "==> Packaging Baudix v$VERSION for Linux (amd64)..."

# 1. Detect Qt prefix
QT_PREFIX=""
if [ -n "$Qt6_DIR" ] && [ -d "$Qt6_DIR" ]; then
    QT_PREFIX="$(cd "$Qt6_DIR/../../.." && pwd)"
elif [ -n "$QT_ROOT_DIR" ] && [ -d "$QT_ROOT_DIR" ]; then
    QT_PREFIX="$QT_ROOT_DIR"
elif [ -n "$QT_DIR" ] && [ -d "$QT_DIR" ]; then
    QT_PREFIX="$(cd "$QT_DIR/../../.." && pwd)"
fi

if [ -z "$QT_PREFIX" ] || [ ! -d "$QT_PREFIX/lib" ]; then
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        QT_CMAKE_DIR=$(grep -E '^(Qt6_DIR|Qt6Core_DIR):PATH=' "$BUILD_DIR/CMakeCache.txt" | head -n1 | cut -d= -f2)
        if [ -n "$QT_CMAKE_DIR" ] && [ -d "$QT_CMAKE_DIR" ]; then
            QT_PREFIX="$(cd "$QT_CMAKE_DIR/../../.." && pwd)"
        fi
    fi
fi

# Fallback: search common runner Qt gcc_64 path
if [ -z "$QT_PREFIX" ] || [ ! -d "$QT_PREFIX/lib" ]; then
    CANDIDATE=$(find /home/runner -type d -name "gcc_64" 2>/dev/null | head -n1)
    if [ -n "$CANDIDATE" ] && [ -d "$CANDIDATE/lib" ]; then
        QT_PREFIX="$CANDIDATE"
    fi
fi

if [ -z "$QT_PREFIX" ] || [ ! -d "$QT_PREFIX/lib" ]; then
    echo "Error: Could not locate Qt installation prefix!"
    exit 1
fi

echo "==> Found Qt prefix: $QT_PREFIX"

# 2. Setup staging area
PKG_DIR="$BUILD_DIR/deb_staging"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/opt/baudix/bin"
mkdir -p "$PKG_DIR/opt/baudix/lib"
mkdir -p "$PKG_DIR/opt/baudix/plugins"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/share/applications"
mkdir -p "$PKG_DIR/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$PKG_DIR/usr/share/pixmaps"
mkdir -p "$PKG_DIR/lib/udev/rules.d"

# 3. Copy application binary
cp "$BUILD_DIR/baudix" "$PKG_DIR/opt/baudix/bin/baudix"
chmod 755 "$PKG_DIR/opt/baudix/bin/baudix"

# 4. Create symlink in /usr/bin
ln -sf /opt/baudix/bin/baudix "$PKG_DIR/usr/bin/baudix"

# 5. Create qt.conf to direct Qt to local lib and plugins
cat << 'EOF' > "$PKG_DIR/opt/baudix/bin/qt.conf"
[Paths]
Prefix = ..
Libraries = lib
Plugins = plugins
EOF
chmod 644 "$PKG_DIR/opt/baudix/bin/qt.conf"

# 6. Copy Desktop Entry, Icons, and Udev Rules
cp "$ROOT_DIR/resources/baudix.desktop" "$PKG_DIR/usr/share/applications/baudix.desktop"
chmod 644 "$PKG_DIR/usr/share/applications/baudix.desktop"

cp "$ROOT_DIR/resources/baudix_icon.svg" "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/baudix.svg"
chmod 644 "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/baudix.svg"

cp "$ROOT_DIR/resources/baudix_icon.svg" "$PKG_DIR/usr/share/pixmaps/baudix.svg"
chmod 644 "$PKG_DIR/usr/share/pixmaps/baudix.svg"

cp "$ROOT_DIR/resources/99-baudix-udev.rules" "$PKG_DIR/lib/udev/rules.d/99-baudix-udev.rules"
chmod 644 "$PKG_DIR/lib/udev/rules.d/99-baudix-udev.rules"

# 7. Copy required Qt libraries (preserving symlinks)
echo "==> Copying Qt shared libraries..."
copy_qt_lib() {
    local lib_name="$1"
    shopt -s nullglob
    for f in "$QT_PREFIX/lib/${lib_name}.so"*; do
        if [ -e "$f" ]; then
            cp -d "$f" "$PKG_DIR/opt/baudix/lib/"
        fi
    done
    shopt -u nullglob
}

# Core required modules for Baudix
copy_qt_lib "libQt6Core"
copy_qt_lib "libQt6Gui"
copy_qt_lib "libQt6Widgets"
copy_qt_lib "libQt6SerialPort"
copy_qt_lib "libQt6Network"
copy_qt_lib "libQt6DBus"
copy_qt_lib "libQt6Svg"
copy_qt_lib "libQt6XcbQpa"
copy_qt_lib "libQt6OpenGL"
copy_qt_lib "libQt6OpenGLWidgets"
copy_qt_lib "libQt6WaylandClient"
copy_qt_lib "libQt6WlShellIntegration"

# Copy ICU libraries if bundled inside Qt prefix
shopt -s nullglob
for f in "$QT_PREFIX/lib/libicu"*.so*; do
    if [ -e "$f" ]; then
        cp -d "$f" "$PKG_DIR/opt/baudix/lib/"
    fi
done
shopt -u nullglob

# 8. Copy required Qt plugins
echo "==> Copying Qt plugins..."
copy_qt_plugin_dir() {
    local plugin_sub="$1"
    if [ -d "$QT_PREFIX/plugins/$plugin_sub" ]; then
        mkdir -p "$PKG_DIR/opt/baudix/plugins"
        cp -a "$QT_PREFIX/plugins/$plugin_sub" "$PKG_DIR/opt/baudix/plugins/"
    fi
}

copy_qt_plugin_dir "platforms"
copy_qt_plugin_dir "imageformats"
copy_qt_plugin_dir "iconengines"
copy_qt_plugin_dir "tls"
copy_qt_plugin_dir "platformthemes"
copy_qt_plugin_dir "xcbglintegrations"
copy_qt_plugin_dir "wayland-graphics-integration-client"
copy_qt_plugin_dir "wayland-shell-integration"

# 9. Strip binaries and shared libraries to reduce package size
if command -v strip >/dev/null 2>&1; then
    echo "==> Stripping symbols to reduce bundle size..."
    strip --strip-unneeded "$PKG_DIR/opt/baudix/bin/baudix" 2>/dev/null || true
    find "$PKG_DIR/opt/baudix/lib" -name "*.so*" -type f -exec strip --strip-unneeded {} + 2>/dev/null || true
    find "$PKG_DIR/opt/baudix/plugins" -name "*.so*" -type f -exec strip --strip-unneeded {} + 2>/dev/null || true
fi

# 10. Generate DEBIAN/control
echo "==> Generating Debian package metadata..."
INSTALLED_SIZE=$(du -sk "$PKG_DIR" | cut -f1)

cat << EOF > "$PKG_DIR/DEBIAN/control"
Package: baudix
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: amd64
Maintainer: hakanyz
Installed-Size: ${INSTALLED_SIZE}
Depends: libc6 (>= 2.35), libgl1, libx11-6, libx11-xcb1, libxkbcommon-x11-0, libxkbcommon0, libxcb-cursor0, libxcb-keysyms1, libxcb-icccm4, libxcb-image0, libxcb-render-util0, libxcb-shape0, libxcb-randr0, libxcb-xinerama0, libxcb-sync1, libfontconfig1, libfreetype6
Description: Professional Serial Terminal for Embedded Systems
 Baudix is a modern, developer-friendly Serial Port Terminal built with C++ and Qt.
 Provides a clean, responsive environment for working with serial communications,
 debugging microcontrollers, and analyzing raw UART streams.
 Features include high-performance async IO, dark theme, macro system, real-time logging,
 and automated background updates.
EOF

# 11. Copy maintainer scripts
cp "$ROOT_DIR/scripts/postinst" "$PKG_DIR/DEBIAN/postinst"
cp "$ROOT_DIR/scripts/prerm" "$PKG_DIR/DEBIAN/prerm"
cp "$ROOT_DIR/scripts/postrm" "$PKG_DIR/DEBIAN/postrm"
chmod 755 "$PKG_DIR/DEBIAN/postinst" "$PKG_DIR/DEBIAN/prerm" "$PKG_DIR/DEBIAN/postrm"

# 12. Build .deb with dpkg-deb
DEB_NAME="baudix_${VERSION}_amd64.deb"
FINAL_DEB="$OUTPUT_DIR/$DEB_NAME"

echo "==> Building Debian package with dpkg-deb: $FINAL_DEB"
dpkg-deb --build --root-owner-group "$PKG_DIR" "$FINAL_DEB"

# Clean up staging directory
rm -rf "$PKG_DIR"

echo "==> Successfully created: $FINAL_DEB"
ls -lh "$FINAL_DEB"
