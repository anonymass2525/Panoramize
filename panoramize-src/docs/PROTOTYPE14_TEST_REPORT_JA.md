# PANORAMA typeGPT Prototype 1.4.0 — 検証報告

検証日：2026-10-30（Linux環境）

## 新規実装
- 新規 `PhaseMatterBridge` ノード（内部型値19、JUCE UI選択値18）。入力Aは12部分音のエネルギー状態。入力Bは Phase Ecology の6位相を12チャンネルへ写像した状態、またはスペクトル系ノードの12位相状態。
- 位相差を使った限定的な周波数引力と、位相相関に基づくエネルギー応答。PCMのミックスではなく状態を受け渡す。ステレオ出力のミュートは内部状態に影響しない。
- `MatterWeave`・`FormantField` から新ノードへの内部状態接続、および新ノードから既存スペクトル処理への再接続。
- 新規57–60番：Phase Mycelium、Iridescent Nerve、Glass Synapse、Living Interference。ファクトリー合計60音色。
- 型制約とDAG検査、GUIノード一覧、ホスト状態保存復元に関するJUCEスモーク試験を追加（Windows未実行）。
- 高域でPhaseEcologyの音声部分音を省略するときも位相フレームを必ず初期化し、読み取り側が未初期化状態を参照しないよう修正。

## 実施した試験と結果

| 試験 | 結果 |
|---|---|
| Linux C++17 Release（GCC）ビルド | 成功 |
| CTest (PANORAMA_DSP / FastMath) | 2/2合格 |
| Phase-Matter Bridge A/B型・DAG制約 | 合格（不正なPCM-only Bやphase-only Aと循環を拒否） |
| PCM-mute検証（A/B音声レベルとも0、OutputはBridgeのみ） | 有音、B位相状態変更でPCM変化を検出 |
| 新4音色×44.1/48/96kHz DSP音声 | CTest合格 |
| 新4音色×3レート×4音同時×2048サンプル ASan/UBSan | 合格（異常検出なし） |
| 旧56プリセット × 3サンプルレート ×4096フレーム ×2ch PCM | 全168条件、1,376,256 float32サンプルが1.3とバイト単位で一致 |
| 新4音色 48kHz/24bit/ステレオ WAV | 各2.8秒、無音なし、RMS 0.056～0.085、ピーク 0.208～0.290 |
| MP3連結プレビュー | 192kbpsでエンコード、デコードエラーなし |
| JUCE/Windows x64 VST3ビルド | 未検証 |
| JUCEホスト状態の保存・再読み込み | テスト実装、Windows未実行 |
| pluginval strictness 5 / Studio One | 未検証 |

## CPU測定（参考値）
Linux仮想CPU、48kHz、1024サンプル（実時間21.33ms相当）。各条件1回のみの参考測定であり、保証値・正規化ベンチマークではありません。

| 音色 | 4音同時処理 | 16音同時処理 |
|---|---:|---:|
| 57 Phase Mycelium | 10.76ms | 58.56ms |
| 58 Iridescent Nerve | 22.98ms | 80.35ms |
| 59 Glass Synapse | 15.70ms | 74.77ms |
| 60 Living Interference | 19.38ms | 74.56ms |

16音時は4音色ともリアルタイム予算を超える。58番は4音でも予算を超える。この処理時間はCPU割当・ビルド設定によって変動する。Studio One実機では再測定が必要。

## 既知の限界
- 固定の9ノードのDAG方式であり、真のゼロ遅延双方向状態結合や物理量の統一状態方程式ではない。
- 6個の位相状態を12バンドへ写像している。12成分すべてが独立したPhase Ecology状態を持つわけではない。
- エイリアシングは高域のテーパー・周波数制限で軽減するが、非線形経路や位相変調を含めゼロではない。
- Windows VST3認識、ホストオートメーション、GUI操作、レイテンシ、複数インスタンス、保存再読込については未検証。
- 旧版の既存プリセットのPCMは保ったが、パッチ編集中の新旧の内部状態の挙動の同一性は保証しない。

## Windows移行
`CMakeLists.txt` / `.github/workflows/windows-vst3.yml` に独立VST3識別子 `Pn14` と `com.independent.panorama.typegpt.v14` を設定。GitHubでWindows x64 Release、JUCE integration、pluginval strictness 5を実行し、最後にStudio Oneで確認する。

**判定：LinuxのDSP研究版として合格。Windows VST3とStudio One実用判定は保留。**
