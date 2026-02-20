#!/bin/bash

HL2SDK_DIR=""
ASIO_DIR=""

if [ -z "$HL2SDK_DIR" ]; then
  echo "Warning: Please set hl2sdk-cs2 path in build.sh before running."
  exit 1
fi

if [ -z "$ASIO_DIR" ]; then
  echo "Warning: Please set asio path in build.sh before running."
  exit 1
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHL2SDK_DIR:PATH="$HL2SDK_DIR" -DASIO_SRC:PATH="$ASIO_DIR"
cmake --build build --config Release
