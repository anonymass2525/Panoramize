# Panoramaize Prototype 2.4.0 — 検証報告

## 改名・移行

- 製品表示名 `Panoramaize v2.4.0`。内部の既存C++識別子と一部のファイル名は、コード互換性のため `panorama` のまま。
- VST3 `PLUGIN_CODE Pz24`、Bundle ID `com.independent.panoramaize.typegpt.v24`。旧2.3から独立した識別子なので旧Studio Oneセッションを置換しない設計。
- JUCE のパラメーターツリー識別子 `PANORAMA` は古い保存状態の手動インポート互換性のため維持。新しい `universalGraphV24` / `universalEnabledV24` を書き、V23キーも読み込む。
- `.pgraph` 新ヘッダー `PANORAMAIZE-GRAPH-24|1`。旧 `PANORAMA-GRAPH-23|1` も読み取り可能。

## 新しい内部状態ポート

- `Synth::acousticStateForNote(note)` により、DSPの実際のモジュラー内部スペクトル・位相・機械状態を読み出せる。旧VAなどの非モジュラー音源では確定している発振位相のみを返す。全音源で全種類のデータが得られるわけではない。
- `UniversalInstrument23::Kind::StateDrive` を追加（コード内列挙7）。Legacy ソース A → UnifiedState ターゲット B。
- Port 0: 位相（turns→sin波による制御）；1: 正規化スペクトルエネルギー；2: 変位（参照m）；3: 速度（参照m/s）。
- 入力レベルは音声サンプルではなく内部状態を参照。変換は能動的な規格化駆動力で、統一状態ソルバーの `addActiveForce` を経て投入される。注入したエネルギーは `EnergyLedger.activeWork` へ加算される。
- このブリッジは厳密なエネルギー保存ポートではなく、規格化した能動変換である。型/領域が非対応なら駆動力0。`prepare` で Legacy/Unified ノード種別と接続妥当性を検証する。

## 動作検証（今回の実行結果）

| 検証 | 結果 |
|---|---|
| Linux C++17 Release全ターゲットビルド | 成功 |
| 選択した10件のCTest | 10/10合格 |
| 旧Universal23及び新StatePorts24の再試験 | 2/2合格 |
| StateDriveの能動/無効比較 | 音声エネルギーの差を確認 |
| Legacy音量0でも位相状態がソルバーに作用 | 確認 |
| スペクトル内部状態の読出し | 確認 |
| 物理・機械状態の読出し | 確認 |
| 能動力仕事のエネルギー会計 | `activeStateWork()` が0でないことを確認 |
| 正しくない型/配線の拒否 | 専用テスト合格 |
| 2.3グラフ旧形式の読込 | 専用テスト合格 |
| 新StatePorts24テストのASan/UBSan | 合格 |
| 残り3件の長時間CTest | 今回の全件合格を確認できず（タイムアウトを含む） |
| JUCE Windows VST3ビルド、pluginval | 未検証 |
| Studio Oneでの2.4認識とGUI | 未検証 |

## 短時間レンダリング実測

48kHz / MIDI 60 / 1.5秒。`Legacy` PCM Level=0でソース音は音声出力されない。統一ソルバーの音を比較。

- 状態接続オフ：RMS 0.00326399、peak 0.0147617。
- 状態接続オン：RMS 0.448723、peak 0.869635。
- 新テストのエネルギー会計：`activeStateWork() ≈ 29.63`（規格化モデルの数値）。
- オーディオクリップを避けるため駆動係数を初期試作より下げた。

## 未達と今後

- すべての旧DSP内部状態の完全な移植は未達。共通グラフ側は旧72音源の互換ブリッジを維持し、状態エクスポートは現在一部ドメインのみ。
- 双方向で保存的な結合、任意の共役ポートの単位変換、ゼロ遅延一般非線形連成、ハードRT完備、多声音演奏の完全なCPU性能は未達。
- GUIは動作する既存パッチエディターを拡張し、タイトルとブランドを変更したもので、以前のデザインコンセプト画像どおりの完成外観ではない。
- Windows / JUCE SDKはLinux実行環境に存在しない。GitHub Actionsの成功とStudio One実機テストが必須。
