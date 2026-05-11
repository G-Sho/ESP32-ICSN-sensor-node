# ICSN for ESP32

ESP32を使用したICSN（Interest-Centric Sensor Network）の実装です。

クリーンアーキテクチャを採用し、レイヤー分離によって保守性と拡張性を重視した設計になっています。このブランチでは通信プロトコルにESP-NOWを使用しています。

## 機能

- **ESP-NOW**を使用したメッシュネットワーク通信
- センサーデータのJSON形式での送受信
- INTEREST/DATA/INVALIDの3つのメッセージタイプ
- 設定可能なネットワークパラメータ（`data/config.json`）
- パフォーマンス測定機能
- クリーンアーキテクチャによる階層設計（Entity、Use Case、Interface、Infrastructure）

## 設定ファイル `data/config.json`

```json
{
    "MAX_PIT_TABLE_SIZE": 20,
    "MAX_CS_TABLE_SIZE": 20,
    "CACHE_ENTRY_TTL_US": 1000000.0,
    "MAX_FIB_TABLE_SIZE": 20,
    "MAX_VIRTUAL_DEPTH": 5,
    "HOP_COUNT_THRESHOLD": 10
}
```

## ビルド環境

- Platform: ESP32
- Framework: Arduino
- IDE: PlatformIO
- 必要なライブラリ:
  - ArduinoJson
  - TaskScheduler
  - DHT sensor library

## ハードウェア

- ESP32開発ボード
- DHTセンサー（温度・湿度）

## 使用方法

1. PlatformIOでプロジェクトをビルド
2. ESP32にアップロード
3. シリアルモニタで動作確認（115200 baud）

---

## Build Profiles

このプロジェクトのビルドプロファイルは以下の3つです。

| Profile | 説明 |
|---------|------|
| `normal` | INFO/WARN/DEBUG を有効化、perf 機能は無効 |
| `perf` | ログ最小化（WARN中心）、perf 機能を有効化 |
| `release` | ログ最小化（WARN中心）、perf 機能は無効 |

### ビルド・書き込み

```bash
# ファイルシステム書き込み
pio run -e normal -t uploadfs

# normal / perf / release のいずれかを選んで書き込み
pio run -e normal  -t upload
pio run -e perf    -t upload
pio run -e release -t upload
```

`upload_port` は `platformio.ini` か CLI の `--upload-port` で指定してください。

### perf コマンドについて

- `perf_stats`
- `perf_reset`
- `dump_perf`
- `reset_perf`
- `perf_count`

上記は `perf` プロファイルでのみ有効です。`normal`/`release` では `{"error": "perf_build_required"}` を返します。

### シリアルコマンド一覧

| コマンド | 説明 |
|---------|-----|
| `send_interest` | INTEREST をブロードキャスト送信開始 |
| `stop_interest` | 定期 INTEREST 送信を停止 |
| `read_sensor` | センサデータを手動送信 |
| `show_counters` | 全ピアの TX/RX カウンタ表示 |
| `show_fib` | FIB（転送情報テーブル）の内容表示 |
| `perf_stats` | パフォーマンス統計表示（perf buildのみ） |
| `perf_reset` | パフォーマンス統計リセット（perf buildのみ） |
| `dump_perf` | センサ計測バッファをJSON出力（perf buildのみ） |
| `reset_perf` | センサ計測バッファをリセット（perf buildのみ） |
| `perf_count` | 計測サンプル数を表示（perf buildのみ） |
| `help` | コマンド一覧表示 |

### 設定ファイル一覧

| ファイル | 用途 |
|---------|-----|
| `data/config.json` | 全プロファイル共通の設定ファイル |
