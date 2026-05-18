---
name: performance-measurement
description: PERFORMANCE_MEASURE と InterestPacketTimingBuffer を使った計測方針。計測ポイント追加、統計表示、オーバーヘッド調整のタスクで参照する。
---

# Performance Measurement

## 目的

性能計測コードを安全に追加し、通常運用への影響を最小化する。

## 基本ルール

- 計測は `PERFORMANCE_MEASURE` 前提で有効化する。
- 通常ビルドでは計測マクロが無効化される前提を崩さない。
- 計測ポイントは意味単位（受信開始、検証終了など）で配置する。

## 推奨実装

- 計測は `InterestPacketTimingBuffer` の `record*()` メソッドを段階的に呼び出す。
  - `recordInterestRx()` → `recordSecurityCheckStart()` → `recordSecurityCheckEnd()` → `recordFibLookup()` → `recordForwardTx()`
- バッファは `ESP-NOWController` が所有し、最大 200 エントリを蓄積する。
- 集計・ダンプも `ESP-NOWController` 経由で行い、`main.cpp` から直接操作しない。

## 非推奨

- 条件コンパイルなしで高頻度計測を常時有効化する。
- 計測のために本来のロジック順序を変更する。
- perf 専用CLIを normal/release で有効化する。

## 変更チェックリスト

- perf ビルドで必要な計測コマンドが機能するか。
- normal/release で不要な処理が抑制されているか。
- 追加した計測ポイント名と目的が明確か。

## 参照先

- `lib/ICSN/performance/InterestPacketTimingBuffer.hpp`
- `lib/ICSN/performance/InterestPacketTimingBuffer.cpp`
- `lib/ICSN/controller/ESP-NOWController.hpp`
- `lib/ICSN/controller/ESP-NOWController.cpp`
- `src/main.cpp`
- `README.md`
