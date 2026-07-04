#!/usr/bin/env python3
"""Bridge: Binance WebSocket -> stdout for LOBSTER C engine."""
import asyncio
import websockets
import sys
import json

async def main():
    url = "wss://stream.binance.com:9443/ws/btcusdt@depth20@100ms"
    async with websockets.connect(url) as ws:
        sys.stderr.write("[CONNECTED] Live BTCUSDT depth stream\n")
        while True:
            msg = await ws.recv()
            sys.stdout.write(msg + "\n")
            sys.stdout.flush()

if __name__ == "__main__":
    asyncio.run(main())
