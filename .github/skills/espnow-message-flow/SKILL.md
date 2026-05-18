---
name: espnow-message-flow
description: ESP-NOW 受信/送信、INTEREST/DATA フロー、InputData/OutputData 変換の指針。通信経路やハンドラ追加、メッセージ処理変更タスクで参照する。
---

# ESP-NOW Message Flow

## 目的

ESP-NOW 通信処理の経路を揃え、分岐追加時の不整合を防ぐ。

## 基本フロー

1. `src/main.cpp` のコールバックで受信する。
2. 必要な検証後に `ESP_NOWControlData` へ変換する。
3. `ESP_NOWController` 経由で `InputBoundary` に渡す。
4. 返却された出力を送信形式へ変換し配信する。

## 実装方針

- `signalCode` 判定は既存定数（INTEREST/DATA）と整合させる。
- 受信側でサイズ・妥当性チェックを先に行う。
- アドレス配列操作は既存のコピー手順に合わせる。
- 新しいメッセージ種別追加時は、InputData/OutputData と UseCase の両方を更新する。

## 非推奨

- 受信コールバック内に大量の業務ロジックを直接書く。
- controller から `UseCaseInteractor` 具象へ直接依存させる。
- 変換前後でフィールドの意味を変える。
- 片方向のみ変更して型定義と処理を不一致にする。

## 変更チェックリスト

- `ESP_NOWControlData` と domain データ間の変換が往復で一貫しているか。
- `hopCount` や `contentName` の更新規則が既存ロジックと一致するか。
- 送信先決定 (`txAddress`) が既存の期待仕様を壊していないか。

## 参照先

- `src/main.cpp`
- `lib/ICSN/controller/ESP-NOWController.hpp`
- `lib/ICSN/controller/ESP-NOWController.cpp`
- `lib/ICSN/controller/ESP-NOWControlData.hpp`
- `lib/ICSN/data_structure/InputData.hpp`
- `lib/ICSN/data_structure/OutputData.hpp`
