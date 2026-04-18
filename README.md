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

## テスト用ブランチ: `copilot/testgw-bridge-sensors-chain`

### 目的

gateway → bridge → sensor(A) → sensor(B) → sensor(C) の多段経路で動作検証するためのテスト環境です。

### ネットワーク構成

```
[gateway]
    |
  (WiFi/LAN)
    |
[bridge]  MAC: 08:D1:F9:37:39:C0
    |
  (ESP-NOW)
    |
[sensor A]  MAC: CC:7B:5C:9A:F3:C4  ← TEST_MAC_A
    |
  (ESP-NOW)
    |
[sensor B]  MAC: CC:7B:5C:9A:F3:AC  ← TEST_MAC_B
    |
  (ESP-NOW)
    |
[sensor C]  MAC: 9C:9C:1F:CF:F4:8C  ← TEST_MAC_C (データソース)
```

### LMK 設定方法

LMK（Local Master Key）は `data/` フォルダの各ロール用 JSON ファイルで管理します。

```json
{
  "PMK": "<32文字の16進数文字列（16バイト）>",
  "LMK": "<32文字の16進数文字列（16バイト）>",
  "peers": [
    {
      "mac": "XX:XX:XX:XX:XX:XX",
      "lmk": "<32文字の16進数文字列>"
    }
  ]
}
```

> ⚠️ **注意**: 実際のキー値はリポジトリにコミットしないでください。  
> 各自のデバイスで使用するキーに書き換えてから LittleFS へ書き込んでください。  
> 上記ファイル内の値はダミーです。

PMK/LMK を変更する場合:
1. 対応する `data/config_*.json` を編集する
2. `pio run -e test_sensor_X -t uploadfs` でファイルシステムを書き込む

### FIB 初期設定

各ノードは起動時に `config_*.json` の `"fib_init"` セクションからルーティング情報を読み込み、FIBに投入します。

| ノード | FIB エントリ | 次ホップ |
|--------|------------|---------|
| Sensor A | `/iot/buildingA/room101/temp` | Sensor B (`cc:7b:5c:9a:f3:ac`) |
| Sensor B | `/iot/buildingA/room101/temp` | Sensor C (`9c:9c:1f:cf:f4:8c`) |
| Sensor C | （なし: データソースのため不要） | — |

FIB エントリの確認はシリアルコマンド `show_fib` で行えます。

`fib_init` の書式:
```json
"fib_init": [
  {
    "content": "/iot/buildingA/room101/temp",
    "next_hop": "cc:7b:5c:9a:f3:ac"
  }
]
```

### A/B/C それぞれのビルド・書き込み・起動方法

#### 1. ファイルシステムの書き込み（全ノード共通）

```bash
# すべてのロール用 config ファイルを LittleFS へアップロード（全ノード共通）
pio run -e test_sensor_a -t uploadfs
```

> `data/` フォルダ内の全ファイル（`config.json`, `config_a.json`, `config_b.json`, `config_c.json`）が LittleFS に書き込まれます。

#### 2. ファームウェアのビルド・書き込み

各センサに書き込むファームウェアは `TEST_NODE_ROLE` ビルドフラグで切り替えます:

```bash
# Sensor A (TEST_NODE_ROLE=1) → /config_a.json を使用
pio run -e test_sensor_a -t upload

# Sensor B (TEST_NODE_ROLE=2) → /config_b.json を使用
pio run -e test_sensor_b -t upload

# Sensor C (TEST_NODE_ROLE=3) → /config_c.json を使用
pio run -e test_sensor_c -t upload
```

`upload_port` は各自の環境に合わせて `platformio.ini` か CLI の `--upload-port` オプションで指定してください。

#### 3. 起動時のシリアルログ確認

起動時に以下のようなログが出力されます:

```
Starting setup...
[ROLE] Sensor A
[SECURITY] Global LMK configured for HMAC
[FIB] Initial entry: /iot/buildingA/room101/temp -> cc:7b:5c:9a:f3:ac
My MAC Address: CC:7B:5C:9A:F3:C4
ESP-NOW initialized successfully
Setup complete.
```

`[FIB] Initial entry` が出力されていれば FIB 初期設定は成功です。

### 動作確認手順

1. **起動順序**: Sensor C → Sensor B → Sensor A → Bridge → Gateway の順に起動する
2. **INTEREST 送信**: Gateway から `/iot/buildingA/room101/temp` に対して INTEREST を送信する
3. **中継確認**: 各センサのシリアルモニタで以下のログを確認する
   - `[RX] <-- (unicast)` → パケット受信
   - `[FIB] Initial entry` (起動時) → FIB 設定済み
   - `[TX] --> XX:XX:XX:XX:XX:XX (unicast)` → 次ホップへ転送
4. **データ返送**: Sensor C がデータを返し、B → A → Bridge → Gateway の順に戻ることを確認する

### シリアルコマンド一覧

| コマンド | 説明 |
|---------|-----|
| `send_interest` | INTEREST をブロードキャスト送信開始 |
| `send_interest_a` | INTEREST を Sensor A 宛に送信開始 |
| `send_interest_b` | INTEREST を Sensor B 宛に送信開始 |
| `stop_interest` | 定期 INTEREST 送信を停止 |
| `read_sensor` | センサデータを手動送信 |
| `show_counters` | 全ピアの TX/RX カウンタ表示 |
| `show_fib` | FIB（転送情報テーブル）の内容表示 |
| `perf_stats` | パフォーマンス統計表示 |
| `perf_reset` | パフォーマンス統計リセット |
| `help` | コマンド一覧表示 |

### 設定ファイル一覧

| ファイル | 用途 |
|---------|-----|
| `data/config.json` | デフォルト設定（`env:esp32dev` 用） |
| `data/config_a.json` | Sensor A 用テスト設定（FIB: A→B） |
| `data/config_b.json` | Sensor B 用テスト設定（FIB: B→C） |
| `data/config_c.json` | Sensor C 用テスト設定（データソース、FIB エントリなし） |