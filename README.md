# ABYSS

**High‑performance limit order book engine and market simulation platform**

A production-grade system that replays historical markets, computes real‑time
microstructure metrics, and will eventually host autonomous trading agents that
learn through interaction — built across C, Python, Scala, Java, and Apache
Spark to demonstrate mastery of the full systems stack.

---

## Status

**Phase 1 complete** — deterministic C core with live market data ingestion

---

## Overview

ABYSS is a multi‑language, multi‑phase engineering project designed to
showcase end‑to‑end systems skills: from hand‑tuned C data structures to
Python machine learning pipelines, JVM services, and distributed computing.

At its heart sits a limit order book matching engine written in C, with
fixed‑point arithmetic, arena‑based memory management, and deterministic
replay. A lightweight Python layer wraps the engine via `ctypes`, and a live
WebSocket bridge streams real order book snapshots from Binance.

Future phases will add a strategy backtesting framework, reinforcement
learning agents, multi‑agent market simulations, and a Scala/Spark
distributed backtesting infrastructure.

---

## Tech Stack

| Layer          | Language   | Role                                      |
|----------------|------------|-------------------------------------------|
| Core engine    | C          | Limit order book, matching, metrics       |
| Bindings       | Python     | ctypes wrapper, strategy framework        |
| Live ingestion | Python + C | WebSocket → stdin → C engine              |
| Distributed    | Scala/Java | Spark ETL, feature store, OMS, risk       |
| API            | Python     | FastAPI service                           |
| Build          | Make       | Auto‑detects Intel/ARM macOS Homebrew     |

---

## Key Features (Phase 1)

- **In‑memory limit order book** — add, cancel, modify, and execute operations
- **Price‑time priority** matching engine with partial fills
- **Fixed‑point arithmetic** throughout — zero floating‑point non‑determinism
- **Memory arena allocator** — all order nodes allocated from a 16 MB pre‑allocated block; no `malloc`/`free` in the hot path
- **Deterministic replay** verified with FNV‑1a 64‑bit hashing (10/10 identical replays)
- **Real‑time microstructure metrics** — midprice, spread, imbalance, microprice
- **Live market data** from Binance WebSocket (BTCUSDT depth, top 20, every 100 ms)
- **Python bindings** via `ctypes` — `AbyssBook` class with context manager support
- **Full test suite** — 20 unit tests across 5 suites, all passing
- **Portable build** — single Makefile works on Intel and Apple Silicon Macs

---

## Getting Started

### Prerequisites

- macOS (Intel or Apple Silicon)
- [Homebrew](https://brew.sh)
- Clang (Xcode Command Line Tools)
- Python 3.10+

### Install Dependencies

```bash
brew install cjson libwebsockets openssl@3
pip3 install websockets   # only needed for the live bridge
