#!/bin/bash

BUILD_DIR="build"
BINARY="$BUILD_DIR/kernel.bin"

if [ ! -f "$BINARY" ]; then
  echo "Warning: missing binary file: '$BINARY' "
  echo "Generating it via make."
fi

cmake --build "$BUILD_DIR" --target kernel.bin
echo "'kernel.bin' generated"

nohup cmake --build "$BUILD_DIR" --target debugg &
QEMU_PID=$!

# Launch gdb on the same file
sleep 1
gdb $BINARY -tui

# kill the current qemu session
kill $QEMU_PID
wait $QEMU_PID 2>/dev/null
