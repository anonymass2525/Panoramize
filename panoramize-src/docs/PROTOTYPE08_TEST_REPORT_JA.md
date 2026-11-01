# PANORAMA typeGPT — Prototype 0.8.0 検証レポート

## 設計判断

0.7.0を独立保存し、従来の音色の完全一致を必須にしない「Spectral Matter」実験へ移行。12の独立した部分音位相・エネルギーを持つ音響源として開発し、部分音間のエネルギー拡散、フォルマント状スペクトル分布、入力Aによる励振、入力Bによるスペクトル形状の変化を統合。非線形処理や変調の完全なアンチエイリアス保証はない。

- プリセット総数: **48**（旧44 + 新規4）。
- 旧音色の意図的な改変: **17 Midnight Geometry**, **25 Midnight Geometry Reforged**, **43 Strange Attractor**。原音を「Spectral Matter」に置き換えたため、旧版のPCMとの一致は期待しない。
- 旧44のうち改変対象を除く**41音色は 44.1/48/96kHz、4096フレーム、ステレオfloat PCMで旧0.7とバイト完全一致**。
- 新規: 45 Opaline Architecture、46 Blooming Inertia、47 Midnight Transmutation、48 Liquid Constellation。
- NodeType 16を追加。既存0–15の値は変更していない。GUIノード選択とJUCE保存復元の新規試験コードを追加。
- VST3コードを **PnG8** / bundle `com.independent.panorama.typegpt.v08` に変更（旧版と別プラグイン）。

## 実行済み検証

| 検証 | 判定 |
|---|---|
| Linux C++17 Release CMakeビルド | PASS |
| CTest DSP テスト（内部アサーションをReleaseでも有効化） | 1/1 PASS |
| Debug ASan / UBSan CTest | 1/1 PASS |
| 48プリセット発音・有限値・ノートイベント・グラフ閉路拒否 | PASS |
| 新4音色、44.1 / 48 / 96kHz、多声音・リセットの決定性 | PASS |
| Spectral Matter入力A / Bのアブレーション | PASS（波形差を検出） |
| 旧版0.7とのPCM回帰比較 | 41一致 / 3意図的相違 |
| 新4音色と再声付け3音色の48kHz / 24bitステレオWAV生成 | PASS |
| MP3試聴版のデコード可能なファイル生成 | PASS |
| Windows VST3コンパイル・JUCE Smoke実行 | **未実施** |
| pluginval Strictness 5（GUI）・Steinberg VST3 validator | **未実施** |
| Studio One 0.8実機 | **未実施** |

## 新4プリセット 48kHz/24bit WAVの測定値

| 番号 | Peak | RMS |
|---|---:|---:|
| 45 | 0.4057 | 0.0703 |
| 46 | 0.4081 | 0.0847 |
| 47 | 0.3937 | 0.0702 |
| 48 | 0.4580 | 0.0980 |

## CPU負荷: 問題あり

Linuxのこの環境で、16同時発音、Release C++、各サンプルレートの**0.25秒分**を合成した実測値。

| プリセット | 48kHzの処理時間 | 96kHzの処理時間 |
|---|---:|---:|
| 17 | 314 ms | 668 ms |
| 25 | 306 ms | 631 ms |
| 43 | 485 ms | 1158 ms |
| 45 | 522 ms | 981 ms |
| 46 | 349 ms | 611 ms |
| 47 | 431 ms | 957 ms |
| 48 | 612 ms | 1195 ms |

0.25秒の実時間予算は250msであり、**測定したすべての16音条件でリアルタイム処理を超過**。実際のStudio OneのCPUは不明。音質と独創性を優先した研究段階だが、最適化を先送りしたまま実用完成と主張しない。初回テストは48kHz・同時2〜4音程度で推奨。

## 制約・既知の課題

- Spectral MatterのNyquist近傍の部分音フェードはエイリアシングを**減らすための措置**であり、FM、波形変形、移動する非線形相互作用について完全帯域制限を意味しない。
- 発音構造の大改変を許可したが、0.8は旧9ノードグラフをベースにした実装であり、今後の理想的な「物理状態／位相／スペクトルを直接交換できる統一音響グラフ」は未完成。
- VST3識別子変更で旧版と併存することを目指したが、Windowsホストが両方を正しく分離するかは未検証。
- 主観的音質はユーザーの試聴で評価。WAV生成だけでは音質向上を証明しない。
- 次: Windowsビルド、JUCE Smoke、pluginval、Studio One、負荷・エイリアシング測定、UI再設計、Sound Space、Inverse Synthesis。
