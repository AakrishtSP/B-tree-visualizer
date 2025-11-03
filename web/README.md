# Web Build

This directory contains files for building the B-Tree Visualizer for web browsers using Emscripten/WebAssembly.

## Building

### Prerequisites

Install [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
# Clone the emsdk repository
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate the latest SDK
./emsdk install latest
./emsdk activate latest

# Set up environment variables (needs to be done in each terminal session)
source ./emsdk_env.sh
```

### Build Instructions

From the project root:

```bash
# Run the build script
./build-web.sh
```

Or manually:

```bash
# Configure
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DPLATFORM=Web

# Build
cmake --build build-web -j4
```

### Testing Locally

After building, serve the files with a local HTTP server:

```bash
cd build-web/bin
python3 -m http.server 8000
```

Then open http://localhost:8000 in your browser.

## Files

- `shell.html` - Custom HTML template with styling and controls info
- `index.html` - Simple redirect to the main application
- `build-web.sh` - Automated build script

## Deployment

The generated files in `build-web/bin/` can be deployed to any static web hosting:
- GitHub Pages
- Netlify
- Vercel
- Any web server

Required files to deploy:
- `btree-raylib.html` (or rename to index.html)
- `btree-raylib.js`
- `btree-raylib.wasm`
- `btree-raylib.data` (contains embedded resources)

## Browser Compatibility

Requires a modern browser with WebAssembly support:
- Chrome/Edge 57+
- Firefox 52+
- Safari 11+
- Opera 44+
