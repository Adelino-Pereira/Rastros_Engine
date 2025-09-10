#!/bin/bash

# Output directory for React/Vite public folder
OUTPUT_DIR=../rastros/public

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

# Compile using Emscripten
em++ \
  bindings.cpp Board.cpp AI.cpp \
  -o ../rastros/public/game.js \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createGameModule" \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=512MB \
  -s MAXIMUM_MEMORY=1GB \
  -s ASSERTIONS=1 \
  -s STACK_SIZE=1MB\
  -std=c++17 \
  -O3 \
  --bind


echo " WebAssembly build complete: game.js and game.wasm are in $OUTPUT_DIR"
