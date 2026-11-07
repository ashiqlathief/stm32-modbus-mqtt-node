#!/usr/bin/env bash
# Open Renode with its GUI and run the firmware. Builds first.
set -euo pipefail
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build
exec renode -e "\$boot=@$PWD/build/bootloader; \$image=@$PWD/build/firmware_signed.bin; \$sensor=@$PWD/sim/si7021.repl; include @$PWD/sim/gui.resc"
