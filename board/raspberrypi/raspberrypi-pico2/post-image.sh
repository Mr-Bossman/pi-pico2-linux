#!/bin/sh
shift
support/scripts/genimage.sh "$@"

if picotool version > /dev/null; then
	picotool uf2 convert "${BINARIES_DIR}/flash-image.bin" \
		"${BINARIES_DIR}/flash-image.uf2" --family rp2350-riscv
else
	echo "picotool not found, skipping uf2 conversion"
	echo "Please run \`picotool uf2 convert ${BINARIES_DIR}/flash-image.bin \
		${BINARIES_DIR}/flash-image.uf2 --family rp2350-riscv\` insead."
fi
tput smso 2>/dev/null
echo "Run \`picotool load -fu ${BINARIES_DIR}/flash-image.uf2\` to flash to pi pico2."
tput rmso 2>/dev/null
