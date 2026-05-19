# Copilot Instructions for ESP32-ICSN Sensor Node

## 前提ルール

- 回答は原則日本語で行う。
- 変更が大きくなる場合は、実装前に短い計画を提示する。
- 1回の変更で影響範囲が広いときは、段階的に分割して実施する。
- 既存アーキテクチャ（Clean Architecture）を優先し、層をまたぐ直接依存を増やさない。

## このプロジェクトの概要

- ESP32 + Arduino + PlatformIO で構成された ICSN (Interest-Centric Sensor Network) 実装。
- 通信は ESP-NOW を利用し、INTEREST/DATA フローで転送する。
- Entity / Use Case / Interface / Infrastructure の層分離を採用する。

## 技術スタック

- PlatformIO (board: esp32dev)
- Arduino framework (C++)
- ESP-NOW, Wi-Fi
- ArduinoJson, TaskScheduler, DHT sensor library

## 主要ディレクトリ

- `src/`: エントリポイントと実行制御
- `lib/ICSN/entity/`: ドメイン値オブジェクト、ルーティング要素
- `lib/ICSN/use_case/`: ユースケース実装
- `lib/ICSN/interface/`: 境界インターフェース
- `lib/ICSN/infrastructure/`: リポジトリ実装
- `lib/ICSN/controller/`: ESP-NOW 連携と入出力変換
- `lib/sensor/`: センサー抽象と実装
- `data/`: 実行時設定 (`config.json`)

## スキル参照ガイド

以下の種類の依頼では、対応する SKILL を優先参照する。

- アーキテクチャ整合性、依存方向、層境界
  - `.github/skills/architecture-clean-layers/SKILL.md`
- ESP-NOW メッセージ処理フロー、Input/Output 変換
  - `.github/skills/espnow-message-flow/SKILL.md`
- HMAC、リプレイ対策、カウンタ検証
  - `.github/skills/security-hmac-counter/SKILL.md`
- ビルドプロファイル、書き込み、運用コマンド
  - `.github/skills/platformio-build-profiles/SKILL.md`
- パフォーマンス計測、PERFORMANCE_MEASURE の扱い
  - `.github/skills/performance-measurement/SKILL.md`
- センサー追加・拡張
  - `.github/skills/sensor-extension/SKILL.md`

## 実装ポリシー

- 依存関係ルールとして、`<I>` は Interface、`<DS>` は DataStructure として扱う。
- 新規機能は、可能な限り `UseCaseInteractor` を中心に振る舞いを追加する。
- 依存方向は以下を維持する。
  - `Controller -> InputBoundary(<I>)`
  - `Controller -> InputData/OutputData(<DS>)`
  - `UseCaseInteractor -> InputBoundary(<I>)`（実装）
  - `UseCaseInteractor -> Data Access Interface(<I>)`
  - `UseCaseInteractor -> Entities`
  - `UseCaseInteractor -> InputData/OutputData(<DS>)`
  - `Data Access(infrastructure) -> Data Access Interface(<I>)`（実装）
- データアクセスは既存の Data Access ポート（`IContentStore` / `IForwardingInformationBase` / `IPendingInterestTable`）と実装クラスの責務を崩さない。
- 直接 `src/main.cpp` にドメインロジックを増やさない。
- ログは `BuildProfile.hpp` の `LOG_*` / `CLI_*` ポリシーに従う。

## 禁止・非推奨

- 層を飛び越えた依存（例: controller から infrastructure へ直接操作）
- controller から `UseCaseInteractor` 具象へ直接依存する変更
- use case から data access 具象へ直接依存する変更（`Data Access Interface(<I>)` 経由にする）
- `InputData/OutputData(<DS>)` を controller-use case 境界以外の都合で肥大化させる変更
- プロファイル条件を無視した perf 専用コードの常時実行
- 既存のセキュリティ検証（HMAC/counter）を省略する変更

## 変更後の確認

- 変更に応じて少なくとも1つの build profile でビルド確認を提案する。
- perf や security に触れた場合は、影響範囲と確認観点を明示する。
