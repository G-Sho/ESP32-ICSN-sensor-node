---
name: overview
description: ESP32-ICSN Sensor Node 全体の目的、適用範囲、不変条件、3軸構成（Build Profile / Board Target / Node Role）を定義する共通ガイド。
---

# ESP32-ICSN Documentation Overview

## 目的

このドキュメントは、SKILL 群に共通する前提を 1 か所に集約し、個別 SKILL を実装ルール中心に保つための上位ガイドである。

## 適用範囲

- `.github/copilot-instructions.md`
- `.github/skills/*/SKILL.md`

## 不変条件

- 個別 SKILL は対象領域の実装ルールに集中し、全体方針の重複記述を最小化する。
- ドキュメントは実装済み事項と将来検討事項を混在させない。
- 表記は相対パスと汎用コマンドを優先し、個人環境依存の絶対パスを避ける。
- ドキュメント整理は、機能コードや設定値の変更を伴わない。

## 3軸モデル

このプロジェクトには次の 3 つの独立軸がある。

1. Build Profile
   - `normal` / `perf` / `release`
   - ログ量と計測有効化を制御する軸。
2. Board Target
   - `esp32dev` / `esp32-s3-devkitc-1-n8r8`
   - ビルド対象ハードウェアを制御する軸。
3. Node Role
   - `sensor` / `cluster_head`
   - ノード容量と実行時設定を制御する軸（`scripts/generate_node_profile.py`）。

3軸は独立して選択できる。運用上は推奨組み合わせを持つが、拘束条件ではない。

## 推奨組み合わせ（補助情報）

- `sensor` ロールは ESP32 系 board target と組み合わせる運用が一般的。
- `cluster_head` ロールは ESP32-S3 系 board target と組み合わせる運用が一般的。

上記は推奨であり、検証・実験のために他の組み合わせを禁止するものではない。

## 参照元（一次ソース）

- `platformio.ini`
- `lib/ICSN/BuildProfile.hpp`
- `scripts/generate_node_profile.py`
