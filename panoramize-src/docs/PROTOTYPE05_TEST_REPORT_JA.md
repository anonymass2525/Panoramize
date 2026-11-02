# PANORAMA typeGPT Prototype 0.5.0 — 検証記録

## 試験したもの（Linux環境）

| 項目 | 判定 |
|---|---|
| GCC 14.2 / C++17 Release DSP CMakeビルド | PASS |
| CTest DSP全体 | 1/1 PASS |
| ASan + UBSan / Debug CTest | 1/1 PASS |
| 旧32プリセット×44.1/48/96kHz×4096ステレオフレーム | 0.4.3 Hotfix2にバイト完全一致 |
| PCM SHA-256（旧／新） | `b79720acd7a936f14f1b6387cf09245f8e9c5b7ba0106f263f1a0f1ded45f3ea` |
| Coupled ModesのA/Bの役割 | 入力Aを切ると無音、Bを切ると波形形状が変化 |
| 各新プリセット、サンプルレート・複数ボイス | 有限値・出力検証PASS |
| JUCEプロセッサ / GUIコンパイル（0.5） | **未実施** |
| Windows VST3 / pluginval（0.5） | **未実施** |
| Studio One実機（0.5） | **未実施** |

※ CTestが1/1でも、テストバイナリ内部には複数のassert系回帰チェックが含まれる。今回は既存モジュラーのエンジン切替テストも保持。

## 新規音色の48kHz 24bit WAV（6.7秒）

| プリセット | Peak | RMS |
|---|---:|---:|
| Tidal Alloy | 0.5188 | 0.1069 |
| Living Lattice | 0.6815 | 0.1341 |
| Glass Metabolism | 0.5208 | 0.1118 |
| Phase Habitat | 0.5405 | 0.1096 |

## CPU調査（Linuxテストコンテナー。実機保証ではない）

16音同時、1秒の音声を単一スレッドで処理した経過時間（最適化ビルド）。

| プリセット | 48kHz | 96kHz |
|---|---:|---:|
| 25 Midnight Geometry Reforged（参考） | 626 ms | 1159 ms |
| 33 Tidal Alloy | 694 ms | 1296 ms |
| 34 Living Lattice | 595 ms | 1294 ms |
| 35 Glass Metabolism | 749 ms | 1417 ms |
| 36 Phase Habitat | 727 ms | 1470 ms |

96kHzかつ16音同時ではリアルタイム処理に必要な1秒を越えた測定があるため、リアルタイム負荷の改善が必要。使用するCPU、最適化、音符やパッチによって結果は異なる。44.1/48kHz、発音数の少ない条件から検証すること。

## 音質上の制約

- 新ノードは安定性とCPU効率を両立した研究試作であり、アンチエイリアシングは未完成。
- プラグインパラメータのリアルタイム自動化時の厳密なサンプル精度保証はない。
- デザイン/音質の聴感評価はユーザーのStudio One実機で行う必要がある。
- Windows実機とJUCE版のビルドはGitHub Actionsで別途検証する。
