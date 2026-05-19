---
name: architecture-clean-layers
description: ESP32-ICSN の Clean Architecture を維持するための指針。層境界、依存方向、責務分離に関するタスク（新規機能追加、責務移動、構造変更）で参照する。
---

# Architecture: Clean Layers

## 目的

このリポジトリの層分離を維持し、拡張時の崩壊を防ぐ。

依存関係ルールとして、`<I>` は Interface、`<DS>` は DataStructure を示す。

## レイヤー責務

- Entity (`lib/ICSN/entity/`)
  - ドメイン値オブジェクトとルーティング要素を表現する。
  - 外部I/Oやハードウェア依存を持たない。
- Use Case (`lib/ICSN/use_case/`)
  - アプリケーションの振る舞いを定義する。
  - `IInputBoundary(<I>)` を実装し、`Data Access Interface(<I>)` と `Entity` に依存する。
  - `InputData/OutputData(<DS>)` を境界データとして扱う。
- Interface (`lib/ICSN/interface/`)
  - Use Case から見たポート（`IInputBoundary`, Data Access ポート）を定義する。
  - 具象実装（controller/infrastructure）には依存しない。
- Data Structure (`lib/ICSN/data_structure/`)
  - Controller と Use Case 間の入出力契約（`InputData`, `OutputData`）を定義する。
  - 通信境界のためのデータ形状のみを持ち、業務判断ロジックを持たない。
- Infrastructure (`lib/ICSN/infrastructure/`)
  - Data Access の具象実装を提供し、Interface の Data Access ポートを実装する。
- Controller (`lib/ICSN/controller/`)
  - 外部入力を `InputData(<DS>)` へ変換し、`IInputBoundary(<I>)` に渡す。
  - 返却された `OutputData(<DS>)` を送信形式へ変換する。
  - `UseCaseInteractor` 具象や infrastructure 具象へ直接依存しない。

## 依存方向（必須）

- Controller -> Interface (`IInputBoundary`)
- Controller -> Data Structure (`InputData`, `OutputData`)
- Use Case Interactor -> Interface (`IInputBoundary` 実装)
- Use Case Interactor -> Interface (Data Access ポート: `IContentStore` / `IForwardingInformationBase` / `IPendingInterestTable`)
- Use Case Interactor -> Entity
- Use Case Interactor -> Data Structure (`InputData`, `OutputData`)
- Infrastructure(Data Access) -> Interface (Data Access ポート実装)
- Entity -> どの外側レイヤーにも依存しない

依存は常に外側から内側へ向ける。実装詳細への直接依存ではなく、境界インターフェース経由で接続する。

## 必須ルール

- 新規ロジック追加時は、まずどの層の責務かを明示する。
- ユースケース追加は `UseCaseInteractor` を起点に設計する。
- 永続・キャッシュ処理は `Data Access Interface(<I>)` 経由に集約する。
- 型変換や通信境界の処理は controller 側で閉じる。
- `InputData/OutputData(<DS>)` は境界契約として維持し、層越え都合のフィールド追加を避ける。

## 非推奨

- `src/main.cpp` にドメイン判断ロジックを追加する。
- controller から `UseCaseInteractor` 具象を直接参照する。
- controller から infrastructure 具象を直接呼び出す。
- use case から infrastructure の具象 Data Access 実装を直接保持・生成する。
- entity 層に Arduino/ESP 固有型を持ち込む。

## 変更チェックリスト

- controller が `IInputBoundary` のみを参照しているか。
- controller が `InputData/OutputData` を境界データとして扱っているか。
- use case が Entity / Interface / DataStructure 以外へ依存していないか。
- infrastructure が Interface ポートを実装する向きになっているか。
- entity が外側レイヤーの型やAPIに依存していないか。
- 既存の責務境界（変換、判断、保存）が混在していないか。
- 追加APIが既存命名と整合しているか。

## 参照先

- `lib/ICSN/use_case/UseCaseInteractor.hpp`
- `lib/ICSN/interface/InputBoundary.hpp`
- `lib/ICSN/interface/data_access/`
- `lib/ICSN/infrastructure/data_access/`
