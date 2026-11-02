# PANORAMA typeGPT Prototype 0.7.0 — 実測検証レポート

## 開発対象と変更

- 0.6の正式なソースZIPを展開し、独立した0.7プロジェクトに派生。
- NodeTypeに **PhaseEcology=15** を末尾追加（既存のenum値は保持）。
- 6部分音の局所位相引力ネットワークを実装。入力A/Bは実時間の音響生成挙動に作用。
- 新音色41〜44を追加。JUCEのChoice、保存復元テスト、36→40→44のプリセット数も更新。
- 0.6 GUIに欠落していた **Resonant Exchange** のノード選択肢を修復。
- Windows VST3の版名とGitHub Actionsを0.7.0に更新。VST3識別コードは旧版と同じ。

## 実施したテスト

| テスト | 結果 |
|---|---|
| Linux GCC C++17 Release CMakeビルド | 合格 |
| CTest PANORAMA_DSP | 1/1 合格（内部のassertはReleaseでも有効） |
| Debug ASan + UBSan / CTest | 1/1 合格 |
| 44プリセットの有限値・非無音・相互差 | 合格 |
| 新4音色、44.1/48/96kHz、複数ボイス | 合格 |
| Phase Ecology入力A・入力Bの個別アブレーション | 出力がそれぞれ変化することを確認 |
| グラフの閉路拒否 | 合格 |
| サンプルレート変更後の再初期化と新規インスタンスの一致 | 合格 |
| 旧40プリセット×3レート×4096フレーム×ステレオ PCM | **120条件バイト完全一致** |
| PCM SHA-256（新・旧とも） | `0bb1eff5ca7f579e9eb2b2621cbf2fdd7552bb2be7c3c05b2d918aa298dc5a38` |
| 新4音色の48kHz/24-bit/stereo WAV | 生成済 |
| 4音色の連続MP3 | 生成済 |
| Windows VST3/ JUCE smoke / pluginval / Studio One | **未実施** |

### 新規WAVの音量

| 41〜44 | Peak | RMS |
|---|---:|---:|
| Harmonic Parliament | 0.5479 | 0.1187 |
| Blue Organism | 0.5647 | 0.1169 |
| Strange Attractor | 0.5841 | 0.1367 |
| Glass Constellation | 0.5166 | 0.1076 |

### CPU負荷（参考値、Linux計測環境・16音同時・1秒分のレンダリング）

| 音色 | 48kHzでの処理時間 | 96kHzでの処理時間 |
|---|---:|---:|
| 41 Harmonic Parliament | 1124 ms | 2062 ms |
| 42 Blue Organism | 1215 ms | 2293 ms |
| 43 Strange Attractor | 1564 ms | 2939 ms |
| 44 Glass Constellation | 1427 ms | 2696 ms |

**警告：16音同時発音はこの測定条件でリアルタイム処理予算を超過。** Windows Studio Oneでのリアルタイム性能は測定していない。負荷はCPU・ビルド設定・同時ノード数に大きく依存。

### 留保

Phase Ecologyは位相引力を用いた生成的DSPであり、実在物体の物理モデルを正確に再現するものではない。瞬間的なFMや音響変調などのエイリアシングは完全には防いでいない。聴感の好みは主観的であり、WAV生成成功と商用品質を同一視しない。JUCEのGUI表示・保存復元のWindows実機検証、旧APVTSパラメーターとの互換性は未検証。
