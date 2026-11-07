#!/usr/bin/env bash
# Build the firmware and run it in Renode for a few seconds. The debug log (USART1) is printed at the end.
# Usage: ./run_sim.sh [seconds]    (default 5)
set -euo pipefail
cd "$(dirname "$0")"
SECS="${1:-5}"
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build
rm -f build/uart1.log
timeout $((SECS + 60)) renode --disable-xwt --console --plain -e \
  "\$boot=@$PWD/build/bootloader; \$image=@$PWD/build/firmware_signed.bin; \$log=@$PWD/build/uart1.log; \$sensor=@$PWD/sim/si7021.repl; include @$PWD/sim/serve.resc; sleep $SECS; quit" \
  < /dev/null > build/renode.out 2>&1
echo "--- debug log (USART1) ---"
cat build/uart1.log
