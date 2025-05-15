#!/bin/bash

set -e  # Exit immediately on any error

# Function to display usage instructions
print_help() {
    echo "Usage: $0 <path_to_art.bin> <base_mac_address>"
    echo "Base MAC address format: XX:XX:XX:XX:XX:XX (hexadecimal)"
    echo "Example: $0 art.bin AA:BB:CC:01:02:03"
    exit 1
}

# Function to validate MAC address format using regex
validate_mac() {
    if [[ ! "$1" =~ ^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$ ]]; then
        echo "Error: Invalid MAC address format."
        exit 1
    fi
}

# Function to split MAC string into array of hex byte strings
mac_to_array() {
    IFS=':' read -ra MAC <<< "$1"
}

# Function to increment the last 3 bytes of a MAC address
# Stops and fails if overflow would require modifying more than 3 least-significant bytes
increment_mac() {
    local -n mac=$1  # Reference the passed-in array by name
    for i in 5 4 3; do
        val=$(( 0x${mac[i]} + 1 ))  # Convert hex to decimal and increment
        if [ $val -le 255 ]; then
            mac[i]=$(printf "%02X" $val)  # Convert back to 2-digit uppercase hex
            return 0
        else
            mac[i]="00"  # Overflow, reset current byte and carry to previous
        fi
    done
    return 1  # Overflowed beyond 3 bytes, invalid
}

# Write a 6-byte MAC address to binary at a given offset
write_mac() {
    local offset_dec=$1  # Offset must be decimal
    shift
    local mac_bytes=("$@")
    for byte in "${mac_bytes[@]}"; do
        printf "\\x$byte"
    done | dd of="$BIN_FILE" bs=1 seek=$offset_dec count=6 conv=notrunc status=none
    (IFS=:; echo "Wrote MAC ${mac_bytes[*]} to offset $(printf "0x%X" $offset_dec)")
}

# Write a 14-character random string (A-Z, 0-9) to binary at a given offset
write_random_string() {
    local offset_dec=$1
    local rand=$(tr -dc 'A-Z0-9' < /dev/urandom | head -c14)
    echo -n "$rand" | dd of="$BIN_FILE" bs=1 seek=$offset_dec count=14 conv=notrunc status=none
    echo "Wrote random ASCII string '$rand' to offset $(printf "0x%X" $offset_dec)"
}

# --- Main Script ---

# Validate arguments or show help
if [ "$#" -ne 2 ] || [[ "$1" == "-h" || "$1" == "--help" ]]; then
    print_help
fi

BIN_FILE="$1"
BASE_MAC="$2"

# Check if file exists
if [ ! -f "$BIN_FILE" ]; then
    echo "Error: File '$BIN_FILE' not found."
    exit 1
fi

# Validate and parse MAC
validate_mac "$BASE_MAC"
mac_to_array "$BASE_MAC"
MAC=("${MAC[@]}")

# Prepare list of MACs (base + 3 increments)
MACS=()
MACS+=( "${MAC[@]}" )
echo "Generating additional MAC addresses..."

for n in 1 2 3; do
    mac_copy=("${MAC[@]}")
    if ! increment_mac mac_copy; then
        echo "Error: Cannot generate 3 consecutive MAC addresses without overflowing first 3 bytes."
        exit 1
    fi
    MACS+=( "${mac_copy[@]}" )
    MAC=("${mac_copy[@]}")
    (IFS=:; echo "Generated MAC+$n: ${mac_copy[*]}")
done

# Write MACs to binary file at required offsets
write_mac $((0x1002)) "${MACS[@]:0:6}"    # base MAC
write_mac $((0x06))   "${MACS[@]:6:6}"    # base +1
write_mac $((0x00))   "${MACS[@]:12:6}"   # base +2
write_mac $((0x5002)) "${MACS[@]:18:6}"   # base +3

# Write random ASCII string at 0x5900
write_random_string $((0x5900))

echo "Binary modification complete."
