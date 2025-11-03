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

# Create build directory
BUILD_DIR="build-web"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Configure with Emscripten
echo "⚙️  Configuring with Emscripten..."
emcmake cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=Web

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
