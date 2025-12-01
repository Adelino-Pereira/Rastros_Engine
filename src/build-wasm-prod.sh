#!/bin/bash
set -e

# Diretório do script
SCRIPT_DIR="$(cd -- "$(dirname "$0")" && pwd)"
# Raiz do repo (uma pasta acima de Enginev3/src)
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
# Pasta public do frontend
OUTPUT_DIR="$REPO_ROOT/rastros/public"
EM_CACHE_DIR="$REPO_ROOT/.emscripten_cache"

mkdir -p "$OUTPUT_DIR"
mkdir -p "$EM_CACHE_DIR"
export EM_CACHE="$EM_CACHE_DIR"

# Build de produção: sem ASSERTIONS, debug a 0, otimização máxima
em++ \
  bindings.cpp Board.cpp AI.cpp HeuristicsUtils.cpp LogMsgs.cpp \
  -o "$OUTPUT_DIR/game.js" \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createGameModule" \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=512MB \
  -s MAXIMUM_MEMORY=1GB \
  -s ASSERTIONS=0 \
  -s STACK_SIZE=1MB \
  -std=c++17 \
  -O3 \
  -DNDEBUG \
  -DDEBUG_LEVEL=0 \
  --bind

echo " WebAssembly prod build complete: game.js and game.wasm are in $OUTPUT_DIR"
