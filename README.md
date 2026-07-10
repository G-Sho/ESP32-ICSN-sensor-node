# ICSN for ESP32

ESP32を使用したICSN（Interest-Centric Sensor Network）の実装です。

クリーンアーキテクチャを採用し、レイヤー分離によって保守性と拡張性を重視した設計になっています。このブランチでは通信プロトコルにESP-NOWを使用しています。

## 機能

- **ESP-NOW**を使用したメッシュネットワーク通信
- センサーデータの送受信
- INTEREST/DATA/INVALIDの3つのメッセージタイプ
- 設定可能なネットワークパラメータ（`data/config.json`）
- パフォーマンス測定機能
- クリーンアーキテクチャによる階層設計（Entity、Use Case、Interface、Infrastructure）

## セキュリティ機能

### ESP-NOW 暗号化（PMK / LMK）

ESP-NOW の WPA2 ベース暗号化を有効化します。

- **PMK (Primary Master Key)**: 全ピア共通のグローバルキー。`config.json` の `PMK` フィールド（32 文字 hex）で設定します。
- **LMK (Local Master Key)**: ピアごとの暗号化キー。`config.json` の `LMK` フィールドをデフォルト値として使用し、ピア登録時に適用します。

### HMAC-SHA256 による送信元認証

各パケットには HMAC-SHA256 ダイジェスト（32 バイト）が付加されます。受信側は LMK を鍵として検証し、改ざんを検出します。

### リプレイ攻撃対策（送受信カウンタ）

`PeerCounterManager` がピアごとに TX/RX カウンタを管理します。受信パケットのカウンタが既知の値以下の場合は破棄することで、録画・再送攻撃を防ぎます。

> **注意**: PMK・LMK はデフォルト値のままでは安全ではありません。本番環境では必ず固有の値に変更してください。

## 依存方向のルール

このプロジェクトの依存方向は以下を基本とします。

- `Controller` → `IInputBoundary`（Interface）
- `Controller` → `IForwardingStateBoundary`（Interface）
- `UseCaseInteractor` → `IInputBoundary` / `IForwardingStateBoundary`（実装）
- `UseCaseInteractor` → Entity
- `UseCaseInteractor` → `IContentStore` / `IForwardingInformationBase` / `IPendingInterestTable`（Data Access Interface）
- `Infrastructure (Data Access)` → `IContentStore` / `IForwardingInformationBase` / `IPendingInterestTable`（実装）

補足:

- `Controller` は `UseCaseInteractor` 具象へ直接依存せず、`IInputBoundary` / `IForwardingStateBoundary` 経由で連携する。
- `UseCaseInteractor` は Data Access 具象（`LRUContentStore` 等）へ直接依存せず、インターフェース経由でアクセスする。
- Entity は外側レイヤー（controller / infrastructure / Arduino 固有 API）に依存しない。

## ノードロール設定（Single Source of Truth）

ノードロールごとの設定は `node_profiles/` を単一情報源とします。

- `node_profiles/sensor.json`
- `node_profiles/cluster_head.json`

PlatformIO の pre-script（`scripts/generate_node_profile.py`）が、ロール設定から以下を生成します。

- Build-time capacity ヘッダ: `.pio/build/<env>/generated/BuildCapacity.hpp`
- Runtime 設定プレビュー: `.pio/build/<env>/generated/config.json`

`node_profiles/*.json` の `build.cs_payload_memory` で、CS payload の優先配置先を指定できます。

- `auto`: ボードに `BOARD_HAS_PSRAM` がある場合は PSRAM 優先、なければ heap
- `psram`: 常に PSRAM 優先（確保失敗時の実運用フォールバックは実装側に依存）
- `heap`: 常に heap

`uploadfs` 実行時は同じ内容が `data/config.json` にも出力され、LittleFS に書き込まれます。

`custom_node_profile`（既定: `sensor`）または環境変数 `ICSN_NODE_PROFILE` でロールを選択できます。

## 設定ファイル `data/config.json`

起動時に LittleFS から読み込まれる実行時設定です。容量（FIB/PIT/CS/RIB）は Build-time に確定するため、`config.json` には runtime パラメータのみを持たせます。

```json
{
  "MAX_VIRTUAL_DEPTH": 5,
  "HOP_COUNT_THRESHOLD": 10,
  "PMK": "0123456789abcdef0123456789abcdef",
  "LMK": "fedcba9876543210fedcba9876543210",
  "peers": [],
  "fib_init": []
}
```

| フィールド | 説明 |
|-----------|------|
| `MAX_VIRTUAL_DEPTH` | 仮想深さの上限（ルーティング制御用） |
| `HOP_COUNT_THRESHOLD` | ホップカウントの上限（ループ抑制） |
| `PMK` | ESP-NOW Global PMK（32 文字 hex = 16 バイト）。全ピア共通の暗号化マスターキー |
| `LMK` | ESP-NOW Local Master Key（32 文字 hex = 16 バイト）。ピア固有暗号化キーのデフォルト値 |
| `peers` | 起動時に登録するピアの MAC アドレス一覧（省略可） |
| `fib_init` | 起動時に投入する FIB 初期エントリ一覧（省略可） |

## ビルド環境

- Platform: ESP32
- Framework: Arduino
- IDE: PlatformIO
- 必要なライブラリ:
  - ArduinoJson
  - TaskScheduler
  - DHT sensor library

## ディレクトリ構成

```
src/                         # エントリポイント（main.cpp）
lib/
  ICSN/
    entity/                  # ドメイン値オブジェクト（ContentName, NodeId, HopCount 等）
    use_case/                # UseCaseInteractor（IInputBoundary / IForwardingStateBoundary 実装）
    interface/               # 境界インターフェース（IInputBoundary, IForwardingStateBoundary,
    │                        #   IContentStore, IForwardingInformationBase, IPendingInterestTable）
    infrastructure/          # Data Access 実装（LRUContentStore, LRUFIBRepository 等）
    controller/              # ESP-NOW 連携（ESP_NOWController, PeerCounterManager）
    data_structure/          # InputData / OutputData（層境界のデータ構造体）
    performance/             # 計測バッファ（InterestPacketTimingBuffer）
    config/                  # Config ローダ
  sensor/                    # センサー抽象と実装（DHT 温度・湿度）
data/
  config.json                # 実行時設定（LittleFS）
```

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

利用可能な PlatformIO 環境は以下のとおりです。

| Environment | Board | 用途 |
|-------------|-------|------|
| `normal` / `perf` / `release` | `ESP32-DevKitC-32E` | センサノード用 |
| `normal_s3` / `perf_s3` / `release_s3` | `ESP32-S3-DevKitC-1-N8R8` | クラスタヘッド用 |

### ビルド・書き込み

```bash
# ファイルシステム書き込み
pio run -e normal -t uploadfs

# ノードロールを切り替えてビルド（例: cluster_head）
set ICSN_NODE_PROFILE=cluster_head
pio run -e normal

# normal / perf / release のいずれかを選んで書き込み
pio run -e normal  -t upload
pio run -e perf    -t upload
pio run -e release -t upload

# ESP32-S3-DevKitC-1-N8R8 向け
pio run -e normal_s3 -t uploadfs

pio run -e normal_s3  -t upload
pio run -e perf_s3    -t upload
pio run -e release_s3 -t upload
```

`upload_port` は `platformio.ini` か CLI の `--upload-port` で指定してください。

CLI で一時的に上書きする場合の例:

```bash
pio run -e normal_s3 -t upload --upload-port COM7
pio run -e normal_s3 -t monitor --monitor-port COM7
```

ESP32-S3-DevKitC-1-N8R8 で書き込みに失敗する場合は、以下を順に確認してください。

- USB ケーブルの挿し直し後に COM ポート番号が変わっていないか
- `platformio.ini` の `upload_port` / `monitor_port` と実機のポートが一致しているか
- 書き込み開始時に `BOOT` を押しながら `RESET` を短く押し、その後 `BOOT` を離してダウンロードモードに入れられるか
- それでも不安定な場合は `--upload-port` に加えて `--upload-speed 115200` を指定して再試行する

S3 のシリアルモニタ例:

```bash
pio run -e normal_s3 -t monitor
```

### perf コマンドについて

- `dump_perf`
- `reset_perf`
- `perf_count`

上記は `perf` プロファイルでのみ有効です。`normal`/`release` では `{"error": "perf_build_required"}` を返します。

### メモリ可視化について

- 起動時に内部RAM/PSRAMのヒープ統計（`total` / `free` / `min`）をシリアルへ出力します。
- `show_mem` コマンドで任意タイミングに同じ統計を取得できます。
- `perf` / `perf_s3` では、30秒ごとに定期メモリログを自動出力します。

補足:

- PlatformIO の RAM 使用率は静的領域中心の指標です。
- `show_mem` は実行時ヒープの実測値（空き量や最小空き量）で、断片化や運用中変動の確認に使います。

### シリアルコマンド一覧

| コマンド | 説明 |
|---------|-----|
| `send_interest` | 宛先 MAC 必須 |
| `stop_interest` | 定期 INTEREST 送信を停止 |
| `read_sensor` | センサデータを手動送信 |
| `show_counters` | 全ピアの TX/RX カウンタ表示 |
| `show_fib` | FIB（転送情報テーブル）の内容表示 |
| `clear_cache` | Content Store と PIT のキャッシュをクリア |
| `show_mem` | 内部RAM/PSRAMヒープ統計（total/free/min）を表示 |
| `dump_perf` | Interest受信パケット処理の計測バッファをJSON出力（perf buildのみ） |
| `reset_perf` | Interest受信パケット処理の計測バッファをリセット（perf buildのみ） |
| `perf_count` | 計測サンプル数を表示（perf buildのみ） |
| `help` | コマンド一覧表示 |

### 設定ファイル一覧

| ファイル | 用途 |
|---------|-----|
| `data/config.json` | 全プロファイル共通の設定ファイル |

起動時に `src/main.cpp` から ICSN ライブラリの初期化 API を呼び出し、
`config.json` の読み込みと設定反映（LMK/FIB 初期エントリ）は ICSN 側で実行されます。

### 起動時の自動動作フラグ

`src/main.cpp` 内の以下の定数で制御します（デフォルトはいずれも `false`）。変更するにはコードを編集して再ビルドします。

| 定数 | デフォルト | 説明 |
|-----|----------|------|
| `AUTO_SENSOR_ENABLED` | `false` | 起動直後から定期センサーデータ送信（10 秒間隔）を開始する |
| `AUTO_INTEREST_ENABLED` | `false` | 宛先 MAC が必要 |

## CI (GitHub Actions)

このリポジトリでは、GitHub Actions で以下を実行します。

- `build` ジョブ: `normal` / `perf` / `release` / `normal_s3` / `perf_s3` / `release_s3` をビルド
- `lint` ジョブ: `normal` / `normal_s3` で `pio check` を実行

CI では実機がないため、`upload` / `uploadfs` は実行しません。

### ローカル再現コマンド

```bash
pio run -e normal
pio run -e perf
pio run -e release
pio run -e normal_s3
pio run -e perf_s3
pio run -e release_s3
pio check -e normal
pio check -e normal_s3
```

## Formatter (C/C++)

このリポジトリでは C/C++（`*.c`, `*.cc`, `*.cpp`, `*.h`, `*.hpp`）のみを
`clang-format` で整形します。

- ルール定義: `.clang-format`
- 除外定義: `.clang-format-ignore`
- CI での必須チェック: `.github/workflows/ci.yml` の `format` ジョブ

推奨バージョン:

- `clang-format-16`

### ローカルで一括整形

PowerShell:

```powershell
$files = git ls-files "*.c" "*.cc" "*.cpp" "*.h" "*.hpp"
if ($files) { clang-format -i --style=file $files }
```

### ローカルでCI同等チェック

PowerShell:

```powershell
$files = git ls-files "*.c" "*.cc" "*.cpp" "*.h" "*.hpp"
if ($files) { clang-format --dry-run --Werror --style=file $files }
```

GitHub Actions (Ubuntu) では `clang-format-16 --dry-run --Werror` を実行し、
差分がある場合はジョブが失敗します。
