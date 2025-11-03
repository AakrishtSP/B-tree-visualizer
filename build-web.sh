#!/bin/bash
# Build script for web version using Emscripten

set -e

echo "🌐 Building B-Tree Visualizer for Web..."

# Check if emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: Emscripten not found!"
    echo "Please install Emscripten SDK first:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

echo "✓ Emscripten found: $(emcc --version | head -n1)"


BUILD_DIR="build-web"
# Check for --clean flag
if [ "$1" == "--clean" ]; then
    echo "🧹 Clean build requested..."
    rm -rf "$BUILD_DIR"
    echo "✓ Removed build directory"
fi

# Setup build directory (keep for caching)
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    echo "📁 Created build directory"
else
    echo "♻️  Using existing build directory for faster incremental builds"
fi

# Enable Emscripten cache
export EM_CACHE="${HOME}/.emscripten_cache"
mkdir -p "$EM_CACHE"
echo "💾 Emscripten cache: $EM_CACHE"

# Check if ccache is available
CMAKE_EXTRA_FLAGS=""
if command -v ccache &> /dev/null; then
    CMAKE_EXTRA_FLAGS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    echo "✓ ccache found, using it for faster builds"
fi

# Configure with Emscripten (only if needed)
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "⚙️  Configuring with Emscripten..."
    emcmake cmake -S . -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DPLATFORM=Web \
        $CMAKE_EXTRA_FLAGS
else
    echo "⚙️  Configuration exists, skipping..."
fi

# Build
echo "🔨 Building..."
cmake --build "$BUILD_DIR" -j$(nproc)

echo ""
echo "✅ Build complete!"
echo ""
echo "📂 Output files in: $BUILD_DIR/bin/"
echo ""
echo "🚀 To test locally, run:"
echo "   cd $BUILD_DIR/bin"
echo "   python3 -m http.server 8000"
echo ""
echo "   Then open: http://localhost:8000"
