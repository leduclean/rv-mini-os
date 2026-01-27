#!/bin/bash

BINARY="kernel.bin"

if [ ! -f "$BINARY" ]; then
  echo "Warning: missing binary file: '$BINARY' "
  echo "Generating it via make."
fi

# Generate binary file via make
make $BINARY >/dev/null
echo "'kernel.bin' generated"

# Launch QEMU in background
nohup make debugg &
QEMU_PID=$!

# Launch gdb on the same file
sleep 1
gdb $BINARY -tui

# kill the current qemu session
kill $QEMU_PID
wait $QEMU_PID 2>/dev/null
