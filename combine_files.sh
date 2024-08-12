#!/bin/bash

# Define constants
OUTPUT_FILE="firmware_image.bin"
FILE_SIZE_MB=16

# Function to create a file filled with 0xFF bytes
create_filled_file() {
    echo "Creating a ${FILE_SIZE_MB} MiB file filled with 0xFF values..."
    dd if=/dev/zero bs=1M count="${FILE_SIZE_MB}" | tr '\000' '\377' > "${OUTPUT_FILE}"
    echo "File ${OUTPUT_FILE} created."
}

# Function to display usage information
usage() {
    echo "Usage: $0 [-h] uboot.bin art.bin openwrt.bin [uboot-env.bin]"
    echo "Combine uboot.bin, art.bin, openwrt.bin, and optionally uboot-env.bin into a single binary file."
    echo "Offsets:"
    echo "  uboot.bin     : Starts at the beginning (offset 0x000000)."
    echo "  uboot-env.bin : Optional, starts at offset 0x050000 (320KB)."
    echo "  art.bin       : Starts at offset 0x060000 (384KB)."
    echo "  openwrt.bin   : Starts at offset 0x070000 (448KB)."
    echo "Options:"
    echo "  -h            Display this help message and exit."
    exit 1
}

# Check if -h option is passed
if [[ "$1" == "-h" ]]; then
    usage
fi

# Check if at least three required arguments are provided
if [ $# -lt 3 ] || [ $# -gt 4 ]; then
    usage
fi

UBOOT_FILE=$1
ART_FILE=$2
OPENWRT_FILE=$3
UBOOT_ENV_FILE=$4

# Ensure the output file exists and is filled with 0xFF bytes
if [ ! -f "$OUTPUT_FILE" ]; then
    create_filled_file
fi

# Append uboot.bin at the beginning
dd if="$UBOOT_FILE" of="$OUTPUT_FILE" bs=512 seek=0 conv=notrunc

# Append uboot-env.bin at offset 0x050000 (320KB) if provided
if [ -n "$UBOOT_ENV_FILE" ]; then
    dd if="$UBOOT_ENV_FILE" of="$OUTPUT_FILE" bs=512 seek=640 conv=notrunc
fi

# Append art.bin at offset 0x060000 (384KB)
dd if="$ART_FILE" of="$OUTPUT_FILE" bs=512 seek=768 conv=notrunc

# Append openwrt.bin at offset 0x070000 (448KB)
dd if="$OPENWRT_FILE" of="$OUTPUT_FILE" bs=512 seek=896 conv=notrunc

echo "Combined image created: $OUTPUT_FILE"
