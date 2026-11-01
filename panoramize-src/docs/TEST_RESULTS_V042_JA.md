# PANORAMA typeGPT — Prototype 0.4.2 検証レポート

実施日：2026-10-27 / Linux x64、GCC 14.2.0 / C++17

## 目的と位置づけ

0.4.1からの限定的な品質修正。音色を新規開発したバージョンではない。ユーザーが希望する最高傑作級の音響生成環境という設計思想は `docs/DESIGN_CHARTER_JA.md` に明文化した。

## 修正した問題

`Synth::prepare(sampleRate)` は、従来サンプルレートとボイス状態だけを更新し、`setPatch()` が事前計算する `cutoffHz_` や `graphFilterCoeffs_` を更新していなかった。ホストがサンプルレート切替後に `setPatch()` を再呼び出ししないDSP API利用経路では、古い係数が残った。

再現テスト：48kHzで `setPatch()` → `prepare(96kHz)` → 再 `setPatch()` なしで発音させた場合と、最初から96kHzで初期化した同一パッチを比較。

- v0.4.1：プリセット19で最大絶対誤差 **0.0217973217**、プリセット30で **0.00361821521**（2048フレーム、左ch）。
- v0.4.2：同じ条件で両プリセットとも最大絶対誤差 **0**。

修正：`prepare()` の末尾で、既存パッチを保持した `setPatch(patch_)` を呼び、サンプルレート依存のフィルター係数とグラフ処理順を更新する。

## 実行した検証

| 項目 | 結果 |
|---|---|
| v0.4.1の素のソースからLinux CMakeビルド | 成功 |
| v0.4.1の既存CTest | 1/1 PASS |
| v0.4.2 Linux C++17 RelWithDebInfo ビルド | 成功 |
| v0.4.2 通常CTest | 1/1 PASS |
| v0.4.2 AddressSanitizer + UndefinedBehaviorSanitizer (Debug, -O1) | 1/1 PASS |
| v0.4.2 再初期化回帰検査 | 5パッチ（19,24,25,30,32）×2048 stereoフレーム、全て一致 |
| 全32プリセット、44.1/48/96kHz、4096フレームの旧版とのfloat32 PCM照合 | SHA-256一致 |
| JUCE GUI / Windows VST3 / pluginval / Studio One | **未検証** |
| 人間による試聴評価・エイリアシング量測定 | **未検証** |

### 回帰検査で照合したPCMハッシュ

- v0.4.1: `4c38e53ef30dfc1b9eae0b0d10cc55fd512e7a432702f1aaca234542526b29ff`
- v0.4.2: `4c38e53ef30dfc1b9eae0b0d10cc55fd512e7a432702f1aaca234542526b29ff`

生成方法：32工場パッチそれぞれについて、音程 `44 + preset%29`（preset は0起算）、ベロシティ0.81、4096 stereo frame、44.1/48/96kHz順に生成したfloat32インターリーブ生PCMバイト列をSHA-256化。

この条件での一致は重要だが、すべてのMIDIシーケンス、GUIオートメーション、サンプルレート切替、CPU負荷条件でのビット完全一致を意味するものではない。

## 音質と実機の未完事項

1. 現在のJUCE VST3ソースをWindows x64で実ビルドし、pluginval strictness 5とStudio Oneでロード・MIDI演奏・GUI操作・プリセット保存復元を確認する。
2. サンプルレート44.1/48/96kHzと64〜1024フレームを実機で試す。DAWの再起動／切替時の状態復元を必ず確認する。
3. VA / FM / PhaseWarp / Wavefolder の高域エイリアシングをスペクトル解析し、聴感・CPUトレードオフで比較する。
4. オーディオコールバックでのヒープ割当、ブロック最悪処理時間、過渡時ポップノイズを実測する。
5. 0.5では型付き音声／制御／イベントルーティングと安全なフィードバックを設計する。大規模改修を勝手に先行させない。

## 保全

`v0.4.0_Source.zip`、`v0.4.1_Hotfix_Source.zip` の原本は変更せず、v0.4.2を新規ディレクトリーとして構築・ZIP化した。Codex版のソースやWAVは流用していない。
