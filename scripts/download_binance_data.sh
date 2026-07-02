#!/bin/bash

SYMBOL=${1:-BTCUSDT}
OUTPUT_DIR="data"

mkdir -p "$OUTPUT_DIR"

OUTFILE="$OUTPUT_DIR/${SYMBOL}_depth_live.json"

echo "Downloading $SYMBOL depth snapshot from Binance API..."
curl -s "https://api.binance.com/api/v3/depth?symbol=${SYMBOL}&limit=5000" -o "$OUTFILE"

if [ ! -s "$OUTFILE" ]; then
    echo "✗ Download failed or file is empty"
    exit 1
fi

echo "✓ Downloaded to $OUTFILE"
file_size=$(ls -lh "$OUTFILE" | awk '{print $5}')
echo "File size: $file_size"
echo ""
echo "Running replay engine..."
./bin/replay "$OUTFILE"
